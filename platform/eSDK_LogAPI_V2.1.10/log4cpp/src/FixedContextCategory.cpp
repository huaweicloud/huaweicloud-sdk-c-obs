/*
 * FixedContextCategory.cpp
 *
 * Copyright 2001, LifeLine Networks BV (www.lifeline.nl). All rights reserved.
 * Copyright 2001, Bastiaan Bakker. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include "PortabilityImpl.hh"
#include <log4cpp/FixedContextCategory.hh>

namespace log4cpp {

    FixedContextCategory::FixedContextCategory(const std::string& name,
                                               const std::string& context) : 
        Category(name, Category::getInstance(name).getParent()),
        _delegate(Category::getInstance(name)),
        _context(context) {
    }

    FixedContextCategory::~FixedContextCategory() {
    }

    void FixedContextCategory::setContext(const std::string& context) {
        _context = context;
    }

    std::string FixedContextCategory::getContext() const {
        return _context;
    }

    Priority::Value FixedContextCategory::getPriority() const LOG4CPP_NOTHROW {
        return Category::getPriority();
    }
   
    Priority::Value FixedContextCategory::getChainedPriority() const LOG4CPP_NOTHROW {
        Priority::Value result = getPriority();

        if (result == Priority::NOTSET) {
            result = _delegate.getChainedPriority();
        }

        return result;
    }
    
    void FixedContextCategory::addAppender(Appender* appender) LOG4CPP_NOTHROW {
        // XXX do nothing for now
    }
    
    void FixedContextCategory::addAppender(Appender& appender) {
        // XXX do nothing for now
    }
    
    Appender* FixedContextCategory::getAppender() const {
        return _delegate.getAppender();
    }
    
    Appender* FixedContextCategory::getAppender(const std::string& name)
    const {
        return _delegate.getAppender(name);
    }

    AppenderSet FixedContextCategory::getAllAppenders() const {
        return _delegate.getAllAppenders();
    }

    void FixedContextCategory::removeAllAppenders() {
        // XXX do nothing for now
    }

    bool FixedContextCategory::ownsAppender() const LOG4CPP_NOTHROW {
        return false;
    }
    
    bool FixedContextCategory::ownsAppender(Appender* appender) const LOG4CPP_NOTHROW {
        return false;
    }
    
    void FixedContextCategory::callAppenders(const LoggingEvent& event)
            LOG4CPP_NOTHROW {
        _delegate.callAppenders(event);
    }

    void FixedContextCategory::setAdditivity(bool additivity) {
        // XXX do nothing for now
    }

    bool FixedContextCategory::getAdditivity() const LOG4CPP_NOTHROW {
        return _delegate.getAdditivity();
    }

    void FixedContextCategory::_logUnconditionally2(Priority::Value priority,
            const std::string& message) LOG4CPP_NOTHROW {
        LoggingEvent event(getName(), message, _context, priority);
        callAppenders(event);
    }
    
} 

