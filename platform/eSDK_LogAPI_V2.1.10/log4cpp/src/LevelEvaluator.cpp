/*
 * Copyright 2002, Log4cpp Project. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <log4cpp/LevelEvaluator.hh>
#include <log4cpp/FactoryParams.hh>
#include <memory>

namespace log4cpp
{
   std::LOG4CPP_UNIQUE_PTR<TriggeringEventEvaluator> create_level_evaluator(const FactoryParams& params)
   {
      std::string level;
      params.get_for("level evaluator").required("level", level);

      return std::LOG4CPP_UNIQUE_PTR<TriggeringEventEvaluator>(new LevelEvaluator(Priority::getPriorityValue(level)));
   }
}

