/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <vector>
#include "rddoc/concurrency/Observer.h"
#include "rddoc/util/RWSpinLock.h"
#include "rddoc/util/SpinLock.h"
#include "rddoc/util/ThreadUtil.h"

namespace rdd {

template <class T> class Observable;

template <class T>
class Subscription {
public:
  Subscription(Subscription&& other) noexcept {
    *this = std::move(other);
  }

  Subscription& operator=(Subscription&& other) noexcept {
    unsubscribe();
    unsubscriber_ = std::move(other.unsubscriber_);
    id_ = other.id_;
    other.unsubscriber_ = nullptr;
    other.id_ = 0;
    return *this;
  }

  ~Subscription() {
    unsubscribe();
  }

private:
  typedef typename Observable<T>::Unsubscriber Unsubscriber;

  Subscription(std::shared_ptr<Unsubscriber> unsubscriber, uint64_t id)
    : unsubscriber_(std::move(unsubscriber)), id_(id) {
    RDDCHECK(id_ > 0);
  }

  void unsubscribe() {
    if (unsubscriber_ && id_ > 0) {
      unsubscriber_->unsubscribe(id_);
      id_ = 0;
      unsubscriber_ = nullptr;
    }
  }

  std::shared_ptr<Unsubscriber> unsubscriber_;
  uint64_t id_{0};

  friend class Observable<T>;
};

template <class T>
class Observable {
public:
  Observable() {}

  Observable(Observable&& other) = delete;

  virtual ~Observable() {
    if (unsubscriber_) {
      unsubscriber_->disable();
    }
  }

  virtual Subscription<T> subscribe(ObserverPtr<T> observer) {
    return subscribeImpl(observer);
  }

protected:
  // Safely execute an operation on each observer. F must take a single
  // Observer<T>* as its argument.
  template <class F>
  void forEachObserver(F f) {
    if (UNLIKELY(!inCallback_)) {
      inCallback_.reset(new bool{false});
    }
    RDDCHECK(!(*inCallback_));
    *inCallback_ = true;

    {
      RWSpinLock::ReadHolder rh(subscribersLock_);
      for (auto& kv : subscribers_) {
        f(kv.second.get());
      }
    }

    if (UNLIKELY((newSubscribers_ && !newSubscribers_->empty()) ||
                 (oldSubscribers_ && !oldSubscribers_->empty()))) {
      {
        RWSpinLock::WriteHolder wh(subscribersLock_);
        if (newSubscribers_) {
          for (auto& kv : *(newSubscribers_)) {
            subscribers_.insert(std::move(kv));
          }
          newSubscribers_->clear();
        }
        if (oldSubscribers_) {
          for (auto id : *(oldSubscribers_)) {
            subscribers_.erase(id);
          }
          oldSubscribers_->clear();
        }
      }
    }
    *inCallback_ = false;
  }

private:
  Subscription<T> subscribeImpl(ObserverPtr<T> observer) {
    auto subscription = makeSubscription();
    typename SubscriberMap::value_type
      kv{subscription.id_, std::move(observer)};
    if (inCallback_ && *inCallback_) {
      if (!newSubscribers_) {
        newSubscribers_.reset(new SubscriberMap());
      }
      newSubscribers_->insert(std::move(kv));
    } else {
      RWSpinLock::WriteHolder{&subscribersLock_};
      subscribers_.insert(std::move(kv));
    }
    return subscription;
  }

  class Unsubscriber {
  public:
    explicit Unsubscriber(Observable* observable) : observable_(observable) {
      RDDCHECK(observable_);
    }

    void unsubscribe(uint64_t id) {
      RDDCHECK(id > 0);
      RWSpinLock::ReadHolder guard(lock_);
      if (observable_) {
        observable_->unsubscribe(id);
      }
    }

    void disable() {
      RWSpinLock::WriteHolder guard(lock_);
      observable_ = nullptr;
    }

  private:
    RWSpinLock lock_;
    Observable* observable_;
  };

  std::shared_ptr<Unsubscriber> unsubscriber_{nullptr};
  SpinLock unsubscriberLock_;

  friend class Subscription<T>;

  void unsubscribe(uint64_t id) {
    if (inCallback_ && *inCallback_) {
      if (!oldSubscribers_) {
        oldSubscribers_.reset(new std::vector<uint64_t>());
      }
      if (newSubscribers_) {
        auto it = newSubscribers_->find(id);
        if (it != newSubscribers_->end()) {
          newSubscribers_->erase(it);
          return;
        }
      }
      oldSubscribers_->push_back(id);
    } else {
      RWSpinLock::WriteHolder{&subscribersLock_};
      subscribers_.erase(id);
    }
  }

  Subscription<T> makeSubscription() {
    if (!unsubscriber_) {
      SpinLockGuard guard(unsubscriberLock_);
      if (!unsubscriber_) {
        unsubscriber_ = std::make_shared<Unsubscriber>(this);
      }
    }
    return Subscription<T>(unsubscriber_, nextSubscriptionId_++);
  }

  std::atomic<uint64_t> nextSubscriptionId_{1};
  RWSpinLock subscribersLock_;
  ThreadLocalPtr<bool> inCallback_;

  typedef std::map<uint64_t, ObserverPtr<T>> SubscriberMap;
  SubscriberMap subscribers_;
  ThreadLocalPtr<SubscriberMap> newSubscribers_;
  ThreadLocalPtr<std::vector<uint64_t>> oldSubscribers_;
};

template <class T>
struct Subject : public Observable<T>, public Observer<T> {
  void on(const T& val) override {
    this->forEachObserver([&](Observer<T>* o){
      o->on(val);
    });
  }
};

template <class T> using ObservablePtr = std::shared_ptr<Observable<T>>;
template <class T> using SubjectPtr = std::shared_ptr<Subject<T>>;

} // namespace rdd
