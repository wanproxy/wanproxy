#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include "wanproxy_config_class_codec.h"

WANProxyConfigClassCodec wanproxy_config_class_codec;

bool
WANProxyConfigClassCodec::activate(ConfigObject *co)
{
	ConfigValue *codeccv = co->members_["codec"];
	if (codeccv == NULL)
		return (true);

	WANProxyConfigTypeCodec *codecct = dynamic_cast<WANProxyConfigTypeCodec *>(codeccv->type_);
	if (codecct == NULL)
		return (false);

	WANProxyConfigCodec codec;
	if (!codecct->get(codeccv, &codec))
		return (false);

	switch (codec) {
	case WANProxyConfigCodecXCodec:
		ERROR("/wanproxy/config/codec") << "Can't configure XCodec just yet.";
		break;
	case WANProxyConfigCodecNone:
		/* Explicitly use no codec.  */
		break;
	default:
		ERROR("/wanproxy/config/codec") << "Invalid codec type.";
		return (false);
	}

	return (true);
}
