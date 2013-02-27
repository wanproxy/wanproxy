/*
 * Copyright (c) 2009-2013 Juli Mallett. All rights reserved.
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

#include "wanproxy_config_class_codec.h"

WANProxyConfigClassCodec wanproxy_config_class_codec;

bool
WANProxyConfigClassCodec::Instance::activate(const ConfigObject *co)
{
	codec_.name_ = co->name_;

	switch (codec_type_) {
	case WANProxyConfigCodecXCodec: {
		/*
		 * XXX
		 * Fetch UUID from permanent storage if there is any.
		 */
		UUID uuid;
		uuid.generate();

		XCodecCache *cache = XCodecCache::lookup(uuid);
		if (cache == NULL) {
			cache = new XCodecMemoryCache(uuid);
			XCodecCache::enter(uuid, cache);
		}
		XCodec *xcodec = new XCodec(cache);

		codec_.codec_ = xcodec;
		break;
	}
	case WANProxyConfigCodecNone:
		codec_.codec_ = NULL;
		break;
	default:
		ERROR("/wanproxy/config/codec") << "Invalid codec type.";
		return (false);
	}

	switch (compressor_) {
	case WANProxyConfigCompressorZlib:
		if (compressor_level_ < 0 || compressor_level_ > 9) {
			ERROR("/wanproxy/config/codec") << "Compressor level must be in range 0..9 (inclusive.)";
			return (false);
		}

		codec_.compressor_ = true;
		codec_.compressor_level_ = compressor_level_;
		break;
	case WANProxyConfigCompressorNone:
		if (compressor_level_ != -1) {
			ERROR("/wanproxy/config/codec") << "Compressor level set but no compressor.";
			return (false);
		}

		codec_.compressor_ = false;
		codec_.compressor_level_ = 0;
		break;
	default:
		ERROR("/wanproxy/config/codec") << "Invalid compressor type.";
		return (false);
	}

	return (true);
}
