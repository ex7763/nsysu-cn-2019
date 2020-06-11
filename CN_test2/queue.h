#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>

template <class T>
class Queue {
protected:
	// Data
	std::queue<T> _queue;
	typename std::queue<T>::size_type _size_max;

	std::mutex _mutex;
	std::condition_variable _fullQue;
	std::condition_variable _empty;

    
	std::atomic_bool _quit; //{ false };
	std::atomic_bool _finished; // { false };

public:
	Queue(const size_t size_max) :_size_max(size_max) {
		_quit = ATOMIC_VAR_INIT(false);
		_finished = ATOMIC_VAR_INIT(false);
	}

	bool push(T& data)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		while (!_quit && !_finished)
		{
			if (_queue.size() < _size_max)
			{
				_queue.push(std::move(data));
				_empty.notify_all();
				return true;
			}
			else
			{
				_fullQue.wait(lock);
			}
		}

		return false;
	}


	bool pop(T &data)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		while (!_quit)
		{
			if (!_queue.empty())
			{
				//data = std::move(_queue.front());
				data = _queue.front();
				_queue.pop();

				_fullQue.notify_all();
				return true;
			}
			else if (_queue.empty() && _finished)
			{
				return false;
			}
			else
			{
				_empty.wait(lock);
			}
		}
		return false;
	}

	// The queue has finished accepting input
	void finished()
	{
		_finished = true;
		_empty.notify_all();
	}

	void quit()
	{
		_quit = true;
		_empty.notify_all();
		_fullQue.notify_all();
	}

	int length()
	{
		return static_cast<int>(_queue.size());
	}
};
