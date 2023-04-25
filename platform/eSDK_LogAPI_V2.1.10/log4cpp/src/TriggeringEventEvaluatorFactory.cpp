/*
 * Copyright 2002, Log4cpp Project. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <log4cpp/TriggeringEventEvaluatorFactory.hh>
#include <stdexcept>

namespace log4cpp
{
   static TriggeringEventEvaluatorFactory* evaluators_factory_ = 0;
   std::LOG4CPP_UNIQUE_PTR<TriggeringEventEvaluator> create_level_evaluator(const FactoryParams& params);

   TriggeringEventEvaluatorFactory& TriggeringEventEvaluatorFactory::getInstance()
   {
      if (!evaluators_factory_)
      {
         std::LOG4CPP_UNIQUE_PTR<TriggeringEventEvaluatorFactory> af(new TriggeringEventEvaluatorFactory);
         af->registerCreator("level", &create_level_evaluator);
         evaluators_factory_ = af.release();
      }

      return *evaluators_factory_;
   }

   void TriggeringEventEvaluatorFactory::registerCreator(const std::string& class_name, create_function_t create_function)
   {
      const_iterator i = creators_.find(class_name);
      if (i != creators_.end())
         throw std::invalid_argument("Creator for Triggering event evaluator with type name '" + class_name + "' allready registered");

      creators_[class_name] = create_function;
   }

   std::LOG4CPP_UNIQUE_PTR<TriggeringEventEvaluator> TriggeringEventEvaluatorFactory::create(const std::string& class_name, const params_t& params)
   {
      const_iterator i = creators_.find(class_name);
      if (i == creators_.end())
         throw std::invalid_argument("There is no triggering event evaluator with type name '" + class_name + "'");

      return (*i->second)(params);
   }

   bool TriggeringEventEvaluatorFactory::registered(const std::string& class_name) const
   {
      return creators_.end() != creators_.find(class_name);
   }
}

