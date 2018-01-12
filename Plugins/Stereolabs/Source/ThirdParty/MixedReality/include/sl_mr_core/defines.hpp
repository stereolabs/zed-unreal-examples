//======= Copyright (c) Stereolabs Corporation, All rights reserved. ===============

#ifndef __DEFINES_H__
#define __DEFINES_H__

#ifdef _WIN32 
	#ifdef  SLMRCORE_EXPORT  
		#define SLMRCORE_API __declspec(dllexport)   
	#else  
		#define SLMRCORE_API __declspec(dllimport)   
	#endif
#else
	#define SLMRUTILS_API
#endif

#include <cstdint>
#include <sl/types.hpp>
#include <sl/Core.hpp>
#include <sl/defines.hpp>

namespace sl {
	namespace mr {

		/**
		 * \enum HMD_DEVICE_TYPE
		 * \ingroup Enumerations
		 * \brief Types of HMD supported
		 */
		enum class HMD_DEVICE_TYPE : uint8_t {
			HMD_DEVICE_TYPE_OCULUS,
			HMD_DEVICE_TYPE_HTC
		};

		/**
		 * \struct noiseFactors
		 * \brief Factors of each channel for noise generation
		 */
		struct SLMRCORE_API noiseFactors {

			noiseFactors(sl::float2 r, sl::float2 g, sl::float2 b)
				:
				r(r),
				g(g),
				b(b)
			{}

			/** r channel */
			sl::float2 r;

			/** g channel */
			sl::float2 g;

			/** b channel */
			sl::float2 b;
		};


		/**
		* \struct trackingData
		* \brief Tracking data
		*/
		struct SLMRCORE_API trackingData {
			/** Path transform from tracking origin ((0, 0, 0) in world space relative to left eye) */
			sl::Transform zedPathTransform;

			/** Zed world space transform ((Location/rotation in world space) with anti drift in stereo) */
			sl::Transform zedWorldTransform;

			/**	Zed world transform without camera offset (Head location/rotation) */
			sl::Transform offsetZedWorldTransform;

			/** Tracking state */
			sl::TRACKING_STATE trackingState;
		};

		typedef sl::timeStamp latencyTime;

		/**
		* \struct keyPose
		* \brief A pair transform/time stamp
		*/
		struct SLMRCORE_API keyPose {
			keyPose()
			{}

			keyPose(sl::Transform transform, sl::timeStamp timeStamp)
				:
				transform(transform),
				timeStamp(timeStamp)
			{}

			/** The pose transform */
			sl::Transform transform;

			/** The pose time stamp */
			sl::timeStamp timeStamp;
		};
	}
}

#endif // __DEFINES_H__