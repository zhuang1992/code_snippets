#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <exception>
#include <ranges>
#include <chrono>


template<typename Derived, typename Key, typename Update>
class worker_pool {
public:
	worker_pool(size_t num_workers): num_workers_(num_workers) {}
	~worker_pool() {
		stop();
	}
	void start() {
		if (!workers_.empty())
			return;

		for (int i = 0; i < num_workers_; ++i) {
			workers_.emplace_back(std::make_unique<std::thread>([this]() {do_work(); }));
		}
	}

	void stop() {
		if (workers_.empty())
			return;
		{
			std::lock_guard lk(work_lock_);
			shutdown_ = true;
		}
		work_cond_.notify_all();
		for (auto& t : workers_) {
			if (t->joinable()) {
				t->join();
			}
		}
		workers_.clear();
	}
	void add_work(Key key, Update update) {
		std::lock_guard<std::mutex> dirty_map_lg{ dirty_map_lock_ };
		auto [it, inserted] = dirty_map_.try_emplace(key, update);
		if (!inserted) {
			it->second.merge(std::move(update));
		}
	}
	size_t process_all() {
		std::lock_guard<std::mutex> dirty_map_lg{ dirty_map_lock_ };
		auto cur_ts = std::chrono::system_clock::now();
		auto num_updates = std::ranges::count_if(dirty_map_, [this, cur_ts](const auto& x) {
			return impl().should_process(x.first, x.second, cur_ts);
			});  // pre-check whether we have work to do. if not, we don't need to fetch work_lock and wip_lock.
		if (!num_updates)
			return 0;
		size_t queued_work{};
		{
			std::scoped_lock lk(work_lock_, wip_lock_);
			for (auto it = dirty_map_.begin(); it != dirty_map_.end();) {
				
				auto& key = it->first;
				auto& update = it->second;
				if (work_in_progress_.contains(key) || !impl().should_process(key, update, cur_ts)) {
					it++;
					continue;
				}
				auto w = dirty_map_.extract(it++); // we need to increase it before doing the extract, otherwise it will be invalidated
				work_queue_.emplace(w.key(), w.mapped());
				work_in_progress_.emplace(key);
				queued_work++;
			}
		}
		if (queued_work) {
			work_cond_.notify_all();
		}
		return queued_work;
	}
private:
	Derived& impl() { return static_cast<Derived&>(*this); }
	void do_work() {
		while (true) {
			std::unique_lock<std::mutex> lk(work_lock_);
			work_cond_.wait(lk, [this]() {return shutdown_ || !work_queue_.empty(); });
			if (shutdown_)
				break;
			auto todo = std::move(work_queue_.front());
			work_queue_.pop();
			lk.unlock();

			try {
				impl().process(todo.first, std::move(todo.second));
				mark_done(todo.first);
			}
			catch (const std::exception& e) {
				std::cout << e.what() << std::endl;
			}
		}
		std::cout << "thread exiting..." << std::endl;
	}

	void mark_done(Key key) {
		std::lock_guard lk(wip_lock_);
		work_in_progress_.erase(key);
	}

	std::unordered_map<Key, Update> dirty_map_;
	std::mutex dirty_map_lock_;

	std::queue<std::pair<Key, Update>> work_queue_;
	std::mutex work_lock_;

	std::unordered_set<Key> work_in_progress_;
	std::mutex wip_lock_;

	size_t num_workers_{};
	std::vector<std::unique_ptr<std::thread>> workers_;

	std::condition_variable work_cond_;
	bool shutdown_{};
};

struct update {
	update(const std::string& s) : val(s) {}
	std::string val{};
	void merge(update u) {
		val = u.val;
	}
};
class demo : public worker_pool<demo, int, update> {
public:
	demo(size_t w) : worker_pool<demo, int, update>(w) {}
	bool should_process(int, update u, const std::chrono::system_clock::time_point& ) {
		return !u.val.empty();
	}
	void process(int k, update u) {
		std::cout << u.val << std::endl;
	}
};

int main() {
	demo wp(10);
	{
		wp.start();
		std::cout << "start done" << std::endl;
		wp.add_work(1, update("a"));
		wp.add_work(2, update("b"));
		std::cout << "add work done" << std::endl;
		std::cout << "add " << wp.process_all() << " job to queue" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		wp.add_work(10, update("c"));

		std::cout << "add " << wp.process_all() << " job to queue" << std::endl;
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	

	return 0;
}