#include <common/buffer.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>

std::map<UUID, XCodecCache *> XCodecCache::cache_map;
