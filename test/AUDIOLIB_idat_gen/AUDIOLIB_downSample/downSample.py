import numpy as np


class AUDIOLIB_downSample:
    """Reference model for AUDIOLIB_downSample.

    Equivalent to MATLAB's downsample(x, n, phase):
        decrease the sample rate by keeping every n-th sample, starting at
        offset `phase`.  Pure decimation -- NO anti-aliasing / low-pass
        filtering (use AUDIOLIB_ssrc for filtered rate conversion).

        downsample([1 2 3 4 5 6 7 8 9], 3)     -> [1 4 7]
        downsample([1 2 3 4 5 6 7 8 9], 3, 1)  -> [2 5 8]
        downsample([1 2 3 4 5 6 7 8 9], 3, 2)  -> [3 6 9]

    Data layout (matches the AUDIOLIB convention, see AUDIOLIB_upsample):
      - planar / non-interleaved (isInterleave=0): in0 is (inChannels, inSamples);
        the sample axis is axis 1.  Output is (inChannels, outSamples).
      - interleaved             (isInterleave=1): in0 is (inSamples, inChannels);
        the sample axis is axis 0.  Output is (outSamples, inChannels).

    where outSamples = ceil((inSamples - phase) / downSampleRatio).  Samples are
    kept along the sample axis in both layouts; channels are decimated
    independently from sample indices phase, phase+n, phase+2n, ...
    """

    def __init__(self, dType="float", inChannels=1, inSamples=1, isInterleave=0, downSampleRatio=1, phase=0):
        """Configure the downsampler.

        Args:
            dType (str): Data type tag ('f32', 'f64'/'d', ...).
            inChannels (int): Number of input channels.
            inSamples (int): Number of input samples per channel.
            isInterleave (int): 0 = planar (channels along axis 0),
                                1 = interleaved (channels along axis 1).
            downSampleRatio (int): Decimation factor n (>= 1).
            phase (int): Sample offset, 0 <= phase < downSampleRatio (MATLAB's 3rd arg).
        """
        if downSampleRatio < 1:
            raise ValueError("downSampleRatio must be >= 1 (got %d)" % downSampleRatio)
        if not (0 <= phase < downSampleRatio):
            raise ValueError(
                "phase must satisfy 0 <= phase < downSampleRatio (got phase=%d, downSampleRatio=%d)"
                % (phase, downSampleRatio)
            )

        self._dType = dType
        self._inChannels = inChannels
        self._inSamples = inSamples
        self._isInterleave = isInterleave
        self._downSampleRatio = downSampleRatio
        self._phase = phase

    def outSamples(self):
        """Number of output samples per channel = ceil((inSamples - phase) / downSampleRatio)."""
        return (self._inSamples - self._phase + self._downSampleRatio - 1) // self._downSampleRatio

    def exec(self, in0):
        """Execute the downsample (decimation) algorithm.

        Args:
            in0 (numpy.ndarray): input in memory layout --
                (inChannels, inSamples) if planar, else (inSamples, inChannels).

        Returns:
            numpy.ndarray: decimated output in the same layout convention,
            same dtype as in0.
        """
        in0 = np.asarray(in0)

        if self._isInterleave:
            # interleaved: rows are time-samples, columns are channels.
            # Keep every n-th row (sample), starting at row `phase` -> axis 0.
            out = in0[self._phase :: self._downSampleRatio, :]
        else:
            # planar: rows are channels, columns are samples.
            # Keep every n-th column (sample), starting at column `phase` -> axis 1.
            out = in0[:, self._phase :: self._downSampleRatio]

        return out


if __name__ == "__main__":
    # --- planar, against the MATLAB doc examples ---
    x = np.array([[1, 2, 3, 4, 5, 6, 7, 8, 9]])
    y0 = AUDIOLIB_downSample(inChannels=1, inSamples=9, downSampleRatio=3, phase=0).exec(x)
    y1 = AUDIOLIB_downSample(inChannels=1, inSamples=9, downSampleRatio=3, phase=1).exec(x)
    y2 = AUDIOLIB_downSample(inChannels=1, inSamples=9, downSampleRatio=3, phase=2).exec(x)
    print("in              :", x.ravel().tolist())
    print("downsample(x,3)  :", y0.ravel().tolist())
    print("downsample(x,3,1):", y1.ravel().tolist())
    print("downsample(x,3,2):", y2.ravel().tolist())
    assert y0.ravel().tolist() == [1, 4, 7]
    assert y1.ravel().tolist() == [2, 5, 8]
    assert y2.ravel().tolist() == [3, 6, 9]
    print("planar OK")

    # --- double (float64) preserves dtype ---
    xd = np.array([[1.5, -2.25, 3.0, 4.0, 5.0]], dtype=np.float64)
    yd = AUDIOLIB_downSample(dType="d", inChannels=1, inSamples=5, downSampleRatio=2, phase=1).exec(xd)
    print("downsample(xd,2,1):", yd.ravel().tolist())
    assert yd.dtype == np.float64
    assert yd.ravel().tolist() == [-2.25, 4.0]
    print("double OK")

    # --- interleaved: (inSamples, inChannels), keep every n-th row ---
    xi = np.array([[1, 2], [3, 4], [5, 6], [7, 8]])  # 4 samples, 2 channels
    yi = AUDIOLIB_downSample(inChannels=2, inSamples=4, downSampleRatio=2, phase=0, isInterleave=1).exec(xi)
    print("interleaved in :", xi.tolist())
    print("interleaved out:", yi.tolist())
    assert yi.tolist() == [[1, 2], [5, 6]]
    # phase=1 keeps the odd sample-rows
    yi1 = AUDIOLIB_downSample(inChannels=2, inSamples=4, downSampleRatio=2, phase=1, isInterleave=1).exec(xi)
    assert yi1.tolist() == [[3, 4], [7, 8]]
    print("interleaved OK")
