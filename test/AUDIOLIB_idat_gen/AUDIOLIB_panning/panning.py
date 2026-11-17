import numpy as np
import sys
import os

# Mock channel mappings dictionary for demonstration purposes.
# Replace with your actual `AUDIOLIB_channelMappings` module import if available.
sys.path.append(
    os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "common")
)
import AUDIOLIB_channelMappings


class AUDIOLIB_panning:
    """
    Python reference model for multichannel panning without frame concepts.
    VBAP     : 2D Vector Based Amplitude Panning with optional Spread Angle
    """

    def __init__(self, dType="float", numSamples=1, numChannels=2, sourceAzimuth=0.0, nextAzimuth = 0.0 , changeFrame = 0,
                  samplingRate=48000, smoothingTime=0.0,
                  speakerNames=None, isInterleave=0, numExecReps=1):
              
        self._dType = dType
        self._numSamples = int(numSamples)
        self._numChannels = int(numChannels)
        self._sourceAzimuth = float(sourceAzimuth)
        self._nextAzimuth= float(nextAzimuth)
        self._nextFrame = int(changeFrame)
        self._samplingRate = int(samplingRate)
        self._smoothingTime = float(smoothingTime)
        self._isInterleave = int(isInterleave)
        self._channelLayout = speakerNames if speakerNames is not None else []
        self._numExecReps = numExecReps


        self._CMAP = AUDIOLIB_channelMappings.AUDIOLIB_CHANNEL_MAP
        self._ANGLE_MAP = AUDIOLIB_channelMappings.AUDIOLIB_CHANNEL_AZIMUTH_MAP

    @staticmethod
    def _normalize_angle(angle):
        angle = float(angle)
        while angle >= 180.0:
            angle -= 360.0
        while angle < -180.0:
            angle += 360.0
        return angle

    @classmethod
    def _angular_distance(cls, angle1, angle2):
        return abs(cls._normalize_angle(angle1 - angle2))

    
    def _calculate_single_point_vbap_gains(self, sourceAzimuth, sorted_indices, sorted_angles):
        finalGains = np.zeros(self._numChannels, dtype=np.float64)
        numSpeakers = len(sorted_indices)

        if numSpeakers == 0:
            return finalGains

        sourceAzimuth = self._normalize_angle(sourceAzimuth)
        sourceRad = np.deg2rad(sourceAzimuth)
        sourceVector = np.array([np.cos(sourceRad), np.sin(sourceRad)], dtype=np.float32)

        sorted_vectors = np.array([
            [np.cos(np.deg2rad(angle)), np.sin(np.deg2rad(angle))]
            for angle in sorted_angles
        ], dtype=np.float32)

        selectedPair = None
        for i in range(numSpeakers):
            j = (i + 1) % numSpeakers
            matrix = np.column_stack((sorted_vectors[i], sorted_vectors[j]))

            if abs(np.linalg.det(matrix)) < 1e-6:
                continue
            gains = np.linalg.solve(matrix, sourceVector)

            if gains[0] >= -1e-10 and gains[1] >= -1e-10:
                selectedPair = (sorted_indices[i], sorted_indices[j], gains)
                break

        if selectedPair is None and numSpeakers > 1:
            matrix = np.column_stack((sorted_vectors[-1], sorted_vectors[0]))
            if abs(np.linalg.det(matrix)) >= 1e-10:
                gains = np.linalg.solve(matrix, sourceVector)
                if gains[0] >= -1e-10 and gains[1] >= -1e-10:
                    selectedPair = (sorted_indices[-1], sorted_indices[0], gains)

        if selectedPair is None:
            distances = np.array([self._angular_distance(sourceAzimuth, angle) for angle in sorted_angles])
            nearestChannel = sorted_indices[int(np.argmin(distances))]
            finalGains[nearestChannel] = 1.0
            return finalGains

        ch1, ch2, gains = selectedPair

        gainNorm = np.sqrt(gains[0] * gains[0] + gains[1] * gains[1])
        if gainNorm > 1e-12:
            gains = gains / gainNorm

        finalGains[ch1] = max(0.0, gains[0])
        finalGains[ch2] = max(0.0, gains[1])
        return finalGains

    def _calculate_vbap_gains(self, sourceAzimuth, sorted_indices, sorted_angles, is_lfe=False, lfe_index=None):
        # if spreadAngle <= 0.0:
        gains = self._calculate_single_point_vbap_gains(sourceAzimuth, sorted_indices, sorted_angles)
        if is_lfe and lfe_index is not None:
            gains[lfe_index] = 0.0
        return gains

    def _calculate_target_gains(self, sourceAzimuth, sorted_indices, sorted_angles, is_lfe=False, lfe_index=None):
        return self._calculate_vbap_gains(sourceAzimuth, sorted_indices, sorted_angles, is_lfe, lfe_index)

    def _generate_smoothed_gains(self, targetGains, previousGains=None):
        gainData = np.zeros((self._numSamples, self._numChannels), dtype=np.float64)

        if previousGains is None:
            previousGains = np.zeros(self._numChannels, dtype=np.float64)

        if self._smoothingTime == 0.0:
            gainData[:, :] = targetGains
            return gainData, targetGains.copy()

        smoothingTimeSec = self._smoothingTime / 1000.0
        coeff = np.exp(-1.0 / (self._samplingRate * smoothingTimeSec))
        alpha = 1.0 - coeff

        for sampleIndex in range(self._numSamples):
            previousGains = coeff * previousGains + alpha * targetGains
            gainData[sampleIndex, :] = previousGains

        return gainData, previousGains

    def exec(self, pInLocal):
        pInLocal = np.asarray(pInLocal, dtype=np.float32)
        
        channel_config_integers = [
            self._CMAP[f"AUDIOLIB_CHANNEL_{name}"] for name in self._channelLayout
        ]

        speaker_angles = np.asarray([
            self._ANGLE_MAP[f"AUDIOLIB_CHANNEL_{name}"]
            for name in self._channelLayout
        ], dtype=np.float64)

        lfe_integer = self._CMAP.get("AUDIOLIB_CHANNEL_LOW_FREQUENCY_EFFECTS")

        # -------- Input validation --------
        if self._isInterleave:
            expectedShape = (self._numExecReps, self._numSamples, 1)
        else:
            expectedShape = (self._numExecReps, 1, self._numSamples)

        if pInLocal.shape != expectedShape:
            raise ValueError(
                f"Input shape {pInLocal.shape} does not match {expectedShape}"
            )

        # -------- Active speaker sorting --------
        active_indices = [
            i for i, ch in enumerate(channel_config_integers)
            if ch != lfe_integer
        ]

        lfe_indices = [
            i for i, ch in enumerate(channel_config_integers)
            if ch == lfe_integer
        ]

        has_lfe = len(lfe_indices) > 0
        lfe_index = lfe_indices[0] if has_lfe else None

        active_angles = speaker_angles[active_indices]
        order = np.argsort(active_angles)

        sorted_angles = active_angles[order]
        sorted_indices = np.array(active_indices)[order]

        # -------- Frame-wise processing --------
        outFrames = []
        gainFrames = []
        targetGainFrames = []
        targetGains = self._calculate_target_gains(
            self._sourceAzimuth,
            sorted_indices,
            sorted_angles,
            is_lfe=has_lfe,
            lfe_index=lfe_index,
        )
        targetGainFrames.append(targetGains)
        previousGains = np.zeros(self._numChannels, dtype=np.float32)

        for frame in range(self._numExecReps):
            if(frame == self._nextFrame - 1):
                targetGains = self._calculate_target_gains(
                            self._nextAzimuth,
                            sorted_indices,
                            sorted_angles,
                            is_lfe=has_lfe,
                            lfe_index=lfe_index,
                        )
                targetGainFrames.append(targetGains)

            gainData, previousGains = self._generate_smoothed_gains(
                targetGains,
                previousGains,
            )

            if self._isInterleave:
                frameIn = pInLocal[frame]                      # (numSamples, 1)
                frameOut = np.tile(frameIn, (1, self._numChannels)) * gainData.astype(np.float32)
            else:

                frameIn = pInLocal[frame]                      # (1, numSamples)
                frameOut = frameIn * gainData.T.astype(np.float32)

            outFrames.append(frameOut)
            gainFrames.append(gainData)

        pOutLocal = np.stack(outFrames, axis=0)
        targetGainDataAll = np.array(
            targetGainFrames
        ) 

        return (
            pOutLocal,
            targetGainDataAll,
            channel_config_integers,
            active_angles,
        )
