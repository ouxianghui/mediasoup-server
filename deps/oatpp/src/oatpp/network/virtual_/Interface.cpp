/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "Interface.hpp"

namespace oatpp { namespace network { namespace virtual_ {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ListenerLock

Interface::ListenerLock::ListenerLock(Interface* interface)
  : m_interface(interface)
{}

Interface::ListenerLock::~ListenerLock() {
  if(m_interface != nullptr) {
    m_interface->unbindListener(this);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interface

std::recursive_mutex Interface::m_registryMutex;
std::unordered_map<oatpp::String, std::weak_ptr<Interface>> Interface::m_registry;

void Interface::registerInterface(const std::shared_ptr<Interface>& interface) {

  std::lock_guard<std::recursive_mutex> lock(m_registryMutex);

  auto it = m_registry.find(interface->getName());
  if(it != m_registry.end()) {
    throw std::runtime_error
    ("[oatpp::network::virtual_::Interface::registerInterface()]: Error. Interface with such name already exists - '" + interface->getName()->std_str() + "'.");
  }

  m_registry.insert({interface->getName(), interface});

}

void Interface::unregisterInterface(const oatpp::String& name) {

  std::lock_guard<std::recursive_mutex> lock(m_registryMutex);

  auto it = m_registry.find(name);
  if(it == m_registry.end()) {
    throw std::runtime_error
      ("[oatpp::network::virtual_::Interface::unregisterInterface()]: Error. Interface NOT FOUND - '" + name->std_str() + "'.");
  }

  m_registry.erase(it);

}

Interface::Interface(const oatpp::String& name)
  : m_name(name)
  , m_listenerLock(nullptr)
{}

Interface::~Interface() {

  unregisterInterface(getName());

  {
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    if ((void*)m_listenerLock != nullptr) {
      OATPP_LOGE("[oatpp::network::virtual_::Interface::~Interface()]",
                 "Error! Interface destructor called, but listener is still bonded!!!");
      m_listenerLock.load()->m_interface = nullptr;
    }
  }

  dropAllConnection();

}

std::shared_ptr<Interface> Interface::obtainShared(const oatpp::String& name) {

  std::lock_guard<std::recursive_mutex> lock(m_registryMutex);

  auto it = m_registry.find(name);
  if(it != m_registry.end()) {
    return it->second.lock();
  }

  std::shared_ptr<Interface> interface(new Interface(name));
  registerInterface(interface);
  return interface;

}

void Interface::ConnectionSubmission::invalidate() {
  m_valid = false;
  m_condition.notify_all();
}

void Interface::ConnectionSubmission::setSocket(const std::shared_ptr<Socket>& socket) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_socket = socket;
  }
  m_condition.notify_one();
}

std::shared_ptr<Socket> Interface::ConnectionSubmission::getSocket() {
  std::unique_lock<std::mutex> lock(m_mutex);
  while (!m_socket && m_valid) {
    m_condition.wait(lock);
  }
  return m_socket;
}

std::shared_ptr<Socket> Interface::ConnectionSubmission::getSocketNonBlocking() {
  if(m_valid) {
    std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
    if(lock.owns_lock()) {
      return m_socket;
    }
  }
  return nullptr;
}

bool Interface::ConnectionSubmission::isValid() {
  return m_valid;
}
  
std::shared_ptr<Socket> Interface::acceptSubmission(const std::shared_ptr<ConnectionSubmission>& submission) {
  
  auto pipeIn = Pipe::createShared();
  auto pipeOut = Pipe::createShared();

  auto serverSocket = Socket::createShared(pipeIn, pipeOut);
  auto clientSocket = Socket::createShared(pipeOut, pipeIn);
  
  submission->setSocket(clientSocket);
  
  return serverSocket;
  
}

std::shared_ptr<Interface::ListenerLock> Interface::bind() {
  std::lock_guard<std::mutex> lock(m_listenerMutex);
  if((void*)m_listenerLock == nullptr) {
    m_listenerLock = new ListenerLock(this);
    return std::shared_ptr<ListenerLock>(m_listenerLock.load());
  }
  throw std::runtime_error(
    "[oatpp::network::virtual_::Interface::bind()]: Can't bind to interface '" + m_name->std_str() + "'. Listener lock is already acquired");
}

void Interface::unbindListener(ListenerLock* listenerLock) {
  std::lock_guard<std::mutex> lock(m_listenerMutex);
  if((void*)m_listenerLock && m_listenerLock == listenerLock) {
    m_listenerLock = nullptr;
    dropAllConnection();
  } else {
    OATPP_LOGE("[oatpp::network::virtual_::Interface::unbindListener()]", "Error! Unbinding wrong listener!!!");
  }
}
  
std::shared_ptr<Interface::ConnectionSubmission> Interface::connect() {
  if((void*)m_listenerLock) {
    auto submission = std::make_shared<ConnectionSubmission>(true);
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_submissions.pushBack(submission);
    }
    m_condition.notify_one();
    return submission;
  }
  return std::make_shared<ConnectionSubmission>(false);
}
  
std::shared_ptr<Interface::ConnectionSubmission> Interface::connectNonBlocking() {
  if((void*)m_listenerLock) {
    std::shared_ptr<ConnectionSubmission> submission;
    {
      std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
      if (lock.owns_lock()) {
        submission = std::make_shared<ConnectionSubmission>(true);
        m_submissions.pushBack(submission);
      }
    }
    if (submission) {
      m_condition.notify_one();
    }
    return submission;
  }
  return std::make_shared<ConnectionSubmission>(false);
}

std::shared_ptr<Socket> Interface::accept(const bool& waitingHandle) {
  std::unique_lock<std::mutex> lock(m_mutex);
  while (waitingHandle && m_submissions.getFirstNode() == nullptr) {
    m_condition.wait(lock);
  }
  if(!waitingHandle) {
    return nullptr;
  }
  return acceptSubmission(m_submissions.popFront());
}

std::shared_ptr<Socket> Interface::acceptNonBlocking() {
  std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
  if(lock.owns_lock() && m_submissions.getFirstNode() != nullptr) {
    return acceptSubmission(m_submissions.popFront());
  }
  return nullptr;
}

void Interface::dropAllConnection() {

  std::unique_lock<std::mutex> lock(m_mutex);

  auto curr = m_submissions.getFirstNode();

  while(curr != nullptr) {
    auto submission = curr->getData();
    submission->invalidate();
    curr = curr->getNext();
  }

  m_submissions.clear();

}

void Interface::notifyAcceptors() {
  m_condition.notify_all();
}
  
}}}
