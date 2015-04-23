/*
 * Copyright (c) 2015 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_cache_disk.h>

#include "wanproxy_config_class_cache.h"

WANProxyConfigClassCache wanproxy_config_class_cache;

bool
WANProxyConfigClassCache::Instance::activate(const ConfigObject *)
{
	WANProxyConfigClassCache::Instance *primary, *secondary;
	XCodecDisk *disk;
	UUID uuid;

	if (uuid_ != "") {
		Buffer uuidbuf(uuid_);
		if (!uuid.decode(&uuidbuf)) {
			ERROR("/wanproxy/config/cache") << "Specified UUID is invalid.";
			return (false);
		}
	}

	switch (type_) {
	case WANProxyConfigCacheMemory:
		if (path_ != "") {
			ERROR("/wanproxy/config/cache") << "No path parameter for memory caches.";
			return (false);
		}
		if (primary_ != NULL || secondary_ != NULL) {
			ERROR("/wanproxy/config/cache") << "Specified cache hierarchy for memory cache.";
			return (false);
		}
		if (uuid_ == "")
			uuid.generate();
		cache_ = new XCodecMemoryCache(uuid, size_);
		break;
	case WANProxyConfigCacheDisk:
		if (uuid_ != "") {
			ERROR("/wanproxy/config/cache") << "Cannot specify UUID for disk cache; a persistent UUID will be allocated.";
			return (false);
		}
		if (primary_ != NULL || secondary_ != NULL) {
			ERROR("/wanproxy/config/cache") << "Specified cache hierarchy for disk cache.";
			return (false);
		}
		if (size_ == 0)
			INFO("/wanproxy/config/cache") << "No disk cache size specified; will attempt to detect from file size.";
		disk = XCodecDisk::open(path_, size_);
		if (disk == NULL) {
			ERROR("/wanproxy/config/cache") << "Could not open disk cache.";
			return (false);
		}
		cache_ = disk->local();
		break;
	case WANProxyConfigCachePair:
		if (uuid_ != "") {
			ERROR("/wanproxy/config/cache") << "Cannot specify UUID for cache pair; secondary UUID will be used.";
			return (false);
		}
		if (size_ != 0) {
			ERROR("/wanproxy/config/cache") << "No size parameter for cache pair.";
			return (false);
		}
		if (primary_ == NULL || secondary_ == NULL) {
			ERROR("/wanproxy/config/cache") << "Cache pair requires both primary and secondary cache.";
			return (false);
		}
		primary = dynamic_cast<WANProxyConfigClassCache::Instance *>(primary_->instance_);
		if (primary == NULL) {
			ERROR("/wanproxy/config/cache") << "Primary cache not properly specified for cache pair.";
			return (false);
		}
		if (primary->cache_ == NULL) {
			ERROR("/wanproxy/config/cache") << "Cache must be activated prior to use in as a primary cache.";
			return (false);
		}
		secondary = dynamic_cast<WANProxyConfigClassCache::Instance *>(secondary_->instance_);
		if (secondary == NULL) {
			ERROR("/wanproxy/config/cache") << "secondary cache not properly specified for cache pair.";
			return (false);
		}
		if (secondary->cache_ == NULL) {
			ERROR("/wanproxy/config/cache") << "Cache must be activated prior to use in as a secondary cache.";
			return (false);
		}
		cache_ = new XCodecCachePair(primary->cache_, secondary->cache_);
		break;
	default:
		ERROR("/wanproxy/config/cache") << "Invalid cache type.";
		return (false);
	}

	uuid_ = cache_->get_uuid().string_;

	return (true);
}
