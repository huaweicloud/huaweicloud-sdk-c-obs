/*
 * Copyright 2002, Log4cpp Project. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#include <log4cpp/PassThroughLayout.hh>
#include <log4cpp/FactoryParams.hh>
#include <memory>

namespace log4cpp
{
   std::LOG4CPP_UNIQUE_PTR<Layout> create_pass_through_layout(const FactoryParams& params)
   {
      return std::LOG4CPP_UNIQUE_PTR<Layout>(new PassThroughLayout);
   }
}
