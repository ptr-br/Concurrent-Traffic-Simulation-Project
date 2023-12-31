#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{

     // perform queue modification under the lock
     std::unique_lock<std::mutex> uLock(_mutex);
     _condition.wait(uLock, [this] { return !this->_queue.empty(); }); // pass unique lock to condition variable
     // remove last vector element from queue
     T msg = std::move(this->_queue.back());
     _queue.pop_back();

     return msg; // will not be copied due to return value optimization (RVO) in C++  
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{

        // perform vector modification under the lock
        std::lock_guard<std::mutex> uLock(_mutex);
        this->_queue.clear();

        // add msg to queue
        this->_queue.push_back(std::move(msg));
        _condition.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    while (true)
    {
        if (this->_queue->receive() == TrafficLightPhase::green) {return; }
    }


}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{

    // get current time 
    auto start = std::chrono::steady_clock::now();
    // get switchDuration from traffic light (4-6 seconds)
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_real_distribution<> distr(4000.0, 6000.0);
    double switchDuration = distr(eng);
    while (true)
    {   
        unsigned long  duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (duration >= switchDuration)
        {      
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // change state of traffic light
            this->_currentPhase = this->_currentPhase == TrafficLightPhase::green ? TrafficLightPhase::red : TrafficLightPhase::green;
            // notify watchers (vehicle queue)
            _queue->send(std::move(_currentPhase));
        }
        // sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        

    }
}

