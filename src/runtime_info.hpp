/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef PIRANHA_RUNTIME_INFO_HPP
#define PIRANHA_RUNTIME_INFO_HPP

#if defined(__linux__)

#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iostream>
#include <string>

extern "C"
{
#include <sys/sysinfo.h>
#include <unistd.h>
}

#elif defined(__FreeBSD__)

extern "C"
{
#include <sys/types.h>
#include <sys/sysctl.h>
}
#include <cstddef>

#elif defined(_WIN32)

extern "C"
{
#include <Windows.h>
}
#include <cstddef>
#include <cstdlib>
#include <new>

#endif

#include <boost/numeric/conversion/cast.hpp>
#include <memory>
#include <thread>

#include "config.hpp"
#include "exceptions.hpp"
#include "runtime_info.hpp"

namespace piranha
{

namespace detail
{

template <typename = int>
struct base_runtime_info
{
	static const std::thread::id m_main_thread_id;
};

template <typename T>
const std::thread::id base_runtime_info<T>::m_main_thread_id = std::this_thread::get_id();

}

/// Runtime information.
/**
 * This class allows to query information about the runtime environment.
 * 
 * @author Francesco Biscani (bluescarni@gmail.com)
 */
class runtime_info: private detail::base_runtime_info<>
{
		using base = detail::base_runtime_info<>;
	public:
		/// Main thread ID.
		/**
		 * @return const reference to an instance of the ID of the main thread of execution.
		 */
		static const std::thread::id &get_main_thread_id()
		{
			return m_main_thread_id;
		}
		// NOTE: here we do not use boost's as we do not want to depend on Boost thread.
		/// Hardware concurrency.
		/**
		 * @return number of detected hardware thread contexts (typically equal to the number of logical CPU cores), or 0 if
		 * the detection fails.
		 * 
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 */
		static unsigned get_hardware_concurrency()
		{
#if defined(__linux__)
			int candidate = ::get_nprocs();
			return (candidate <= 0) ? 0u : static_cast<unsigned>(candidate);
#elif defined(__FreeBSD__)
			int count;
			std::size_t size = sizeof(count);
			return ::sysctlbyname("hw.ncpu",&count,&size,NULL,0) ? 0u : static_cast<unsigned>(count);
#elif defined(_WIN32)
			::SYSTEM_INFO info = ::SYSTEM_INFO();
			::GetSystemInfo(&info);
			// info.dwNumberOfProcessors is a dword, i.e., a long unsigned integer.
			// http://msdn.microsoft.com/en-us/library/cc230318(v=prot.10).aspx
			unsigned retval = 0u;
			try {
				retval = boost::numeric_cast<unsigned>(info.dwNumberOfProcessors);
			} catch (...) {
				// Do not do anything, just keep zero.
			}
			return retval;
#else
			return 0u;
#endif
		}
		/// Size of the data cache line.
		/**
		 * @return data cache line size (in bytes), or 0 if the value cannot be determined.
		 * 
		 * @throws std::system_error in case of failure(s) by threading primitives.
		 */
		static unsigned get_cache_line_size()
		{
#if defined(__linux__)
			auto ls = ::sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
			// This can fail on some systems, resort to try reading the /sys entry.
			// NOTE: here we could iterate over all cpus and take the maximum cache line size.
			if (!ls) {
				std::ifstream sys_file("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size");
				if (sys_file.is_open() && sys_file.good()) {
					try {
						std::string line;
						std::getline(sys_file,line);
						ls = boost::lexical_cast<decltype(ls)>(line);
					} catch (...) {}
				}
			}
			if (ls > 0) {
				unsigned retval = 0u;
				try {
					retval = boost::numeric_cast<unsigned>(ls);
				} catch (...) {}
				return retval;
			} else {
				return 0u;
			}
#elif defined(_WIN32) && defined(PIRANHA_HAVE_SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
			// Adapted from:
			// http://strupat.ca/2010/10/cross-platform-function-to-get-the-line-size-of-your-cache
			std::size_t line_size = 0u;
			::DWORD buffer_size = 0u;
			::SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buffer = 0;
			// This is expected to fail, with a specific error code.
			const auto retval = ::GetLogicalProcessorInformation(0,&buffer_size);
			if (retval || ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
				return 0u;
			}
			try {
				buffer = (::SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)std::malloc(boost::numeric_cast<std::size_t>(buffer_size));
				if (unlikely(!buffer)) {
					piranha_throw(std::bad_alloc,);
				}
				if (!::GetLogicalProcessorInformation(&buffer[0u],&buffer_size)) {
					std::free(buffer);
					return 0u;
				}
				for (::DWORD i = 0u; i != buffer_size / sizeof(::SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
					if (buffer[i].Relationship == ::_LOGICAL_PROCESSOR_RELATIONSHIP::RelationCache && buffer[i].Cache.Level == 1) {
						line_size = buffer[i].Cache.LineSize;
						break;
					}
				}
				std::free(buffer);
				return boost::numeric_cast<unsigned>(line_size);
			} catch (...) {}
			return 0u;
#else
			// TODO: FreeBSD, OSX, etc.?
			return 0u;
#endif
		}
};

}

#endif
