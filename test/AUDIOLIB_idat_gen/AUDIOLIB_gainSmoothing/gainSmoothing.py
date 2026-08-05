import numpy as np

class AUDIOLIB_gainSmoothing:
    """Applies gain ramp across audio frames with strict dtype enforcement."""
    
    def __init__(self, dType="float", inSamples=1, inChannels=1, currentGain=1.0, 
                 targetGain=1.0, samplingRate=48000.0, fadeTime=0.0, isInterleaved=False, 
                 fadeType="SYMMETRIC", numExecReps=1):
        
        self._dType = dType.lower()
        self._npDtype = np.float64 if self._dType == "double" else np.float32
        
        self._inSamples = int(inSamples)
        self._inChannels = int(inChannels)
        self._currentGain = float(currentGain)
        self._targetGain = float(targetGain)
        self._samplingRate = float(samplingRate)
        self._fadeTime = float(fadeTime)
        self._isInterleaved = bool(isInterleaved)
        self._fadeType = fadeType.upper()
        self._numExecReps = int(numExecReps)
        
        self._frameSize = self._inSamples  
        self._samplesPerFrame = self._inSamples * self._inChannels
        self._totalSamplesPerChannel = self._inSamples * self._numExecReps

    def _compute_symmetric_gain(self) -> np.ndarray:
        targetGain = self._targetGain
        currentGain = self._currentGain
        total_samples = self._totalSamplesPerChannel
        
        if currentGain <= 0 or targetGain <= 0:
            raise ValueError("Symmetric fading requires positive gains.")

        timeConstantSec = np.float32(self._fadeTime / 1000.0)
        totalFadeSamples = np.float32(timeConstantSec * np.float32(self._samplingRate))

        # Pre-fill pCurrentGain array using np.float32
        pCurrentGain = np.full(total_samples, np.float32(targetGain), dtype=np.float32)

        if totalFadeSamples > np.float32(0.0) and self._fadeTime > 0:
            fade_len = min(int(totalFadeSamples), total_samples)
            
            # Force single-precision gain bounds
            curr_gain_f32 = np.float32(currentGain)
            targ_gain_f32 = np.float32(targetGain)
            
            gain_ratio = np.float32(targ_gain_f32 / curr_gain_f32)
            inv_samples = np.float32(np.float32(1.0) / totalFadeSamples)
            gainStep = np.float32(np.power(gain_ratio, inv_samples))

            # Iterative multiplicative accumulation loop matching C single-precision rounding
            gain_curve = np.empty(fade_len, dtype=np.float32)
            accum_gain = curr_gain_f32

            for s in range(fade_len):
                accum_gain = np.float32(accum_gain * gainStep)
                gain_curve[s] = accum_gain

            # Clamping behavior using float32 bounds
            if gainStep > np.float32(1.0):
                gain_curve = np.minimum(gain_curve, targ_gain_f32)
            else:
                gain_curve = np.maximum(gain_curve, targ_gain_f32)

            pCurrentGain[:fade_len] = gain_curve
            pCurrentGain[fade_len:] = targ_gain_f32
            
        return pCurrentGain

    def _compute_smooth_gain(self) -> np.ndarray:
        targetGain = self._npDtype(self._targetGain)
        total_samples = self._totalSamplesPerChannel
        pCurrentGain = np.zeros(total_samples, dtype=self._npDtype)
        gainThreshold = self._npDtype(1e-6)

        timeConstantSec = self._fadeTime / 1000.0
        
        if self._fadeTime <= 0.0:
            pCurrentGain[:] = targetGain
            return pCurrentGain

        alpha = self._npDtype(np.exp(-1.0 / (self._samplingRate * timeConstantSec)))
        one_minus_alpha = self._npDtype(1.0) - alpha
        
        currentGain = self._npDtype(self._currentGain)
        for frame in range(self._numExecReps):
            startIdx = frame * self._frameSize
            endIdx = startIdx + self._frameSize

            if frame > 0 and abs(pCurrentGain[startIdx - 1] - targetGain) < gainThreshold:
                pCurrentGain[startIdx:endIdx] = targetGain
                currentGain = targetGain
            else:
                for j in range(startIdx, endIdx):
                    currentGain = alpha * currentGain + one_minus_alpha * targetGain
                    pCurrentGain[j] = currentGain
        # targetGain = np.float32(self._targetGain)
        # total_samples = self._totalSamplesPerChannel
        # pCurrentGain = np.zeros(total_samples, dtype=np.float32)
        # gainThreshold = np.float32(1e-6)

        # timeConstantSec = np.float32(self._fadeTime / 1000.0)

        # if self._fadeTime <= 0.0:
        #     pCurrentGain[:] = targetGain
        #     return pCurrentGain

        # # expf(-1.0f / (samplingRate * timeConstantSec)) using strictly single precision
        # tau_samples = np.float32(np.float32(self._samplingRate) * timeConstantSec)
        # alpha = np.float32(np.exp(np.float32(-1.0) / tau_samples))
        # one_minus_alpha = np.float32(np.float32(1.0) - alpha)
        # print(alpha , one_minus_alpha)

        # currentGain = np.float32(self._currentGain)

        # for frame in range(self._numExecReps):
        #     startIdx = frame * self._frameSize
        #     endIdx = startIdx + self._frameSize

        #     # Use np.abs and np.float32 comparison for thresholding
        #     if frame > 0 and np.abs(np.float32(pCurrentGain[startIdx - 1] - targetGain)) < gainThreshold:
        #         pCurrentGain[startIdx:endIdx] = targetGain
        #         currentGain = targetGain
        #     else:
        #         # Iterative scalar accumulation matching C floating-point hardware registers
        #         for j in range(startIdx, endIdx):
        #             currentGain = np.float32(alpha * currentGain + one_minus_alpha * targetGain)
        #             pCurrentGain[j] = currentGain

        return pCurrentGain

    def exec(self, pInLocal: np.ndarray):
        pInLocal = np.asarray(pInLocal, dtype=self._npDtype)
        
        if self._fadeType == "SYMMETRIC":
            pCurrentGain = self._compute_symmetric_gain()
        elif self._fadeType == "SMOOTH":
            pCurrentGain = self._compute_smooth_gain()
        else:
            pCurrentGain = np.full(self._totalSamplesPerChannel, self._targetGain, dtype=self._npDtype)

        gain_per_frame = pCurrentGain.reshape(self._numExecReps, self._inSamples)
        # last_val = gain_per_frame[-1, -1]
        # print(last_val , self._targetGain , self._currentGain)
        # if(last_val != self._targetGain):
        #     print("Not sufficient")

        if self._isInterleaved:
            gain_expanded = np.repeat(gain_per_frame, self._inChannels, axis=1)
        else:
            gain_expanded = np.tile(gain_per_frame, (1, self._inChannels))

        pInLocal_2d = pInLocal.reshape(self._numExecReps, self._samplesPerFrame)
        pOutLocal = pInLocal_2d * gain_expanded

        # Return NumPy arrays directly (flattened) to preserve exact float32 dtype
        return pOutLocal, pCurrentGain