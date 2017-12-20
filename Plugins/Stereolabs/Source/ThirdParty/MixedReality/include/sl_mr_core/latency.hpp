#ifndef __LATENCY_H__
#define __LATENCY_H__

#include "sl_mr_core\defines.hpp"

namespace sl {
	namespace mr {

		/**
		 * \brief Initialize latency corrector. No functions can be called before initialization.
		 */
		SLMRCORE_API void latencyCorrectorInitialize();

		/**
		 * \brief Shutdown latency corrector. No functions can be called after shutdown.
		 */
		SLMRCORE_API void latencyCorrectorShutdown();

		/**
		 * \brief Delete latency corrector
		 * @param keyPose		   The key
		 */
		SLMRCORE_API void latencyCorrectorAddKeyPose(const sl::mr::keyPose& keyPose);

		/**
		 * \brief Delete latency corrector
		 * @param timeStamp        Time stamp corresponding transform
		 * @param outTransform     Time latency transform
		 * @return True if the transform is retrieved
		 */
		SLMRCORE_API bool latencyCorrectorGetTransform(sl::timeStamp timeStamp, sl::Transform& outTransform, bool useLatencyTime = true);

	}
}

#endif // __LATENCY_H__ 