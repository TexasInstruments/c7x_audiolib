import numpy as np


class AUDIOLIB_upSample:
    """Reference model for AUDIOLIB_upSample.

    Equivalent to MATLAB's upsample(x, n, phase):
        increase the sample rate by inserting (n - 1) zeros between samples,
        optionally offset by `phase` samples.  Pure zero-insertion -- NO
        anti-imaging/interpolation filtering (use AUDIOLIB_ssrc/asrc for that).

        upsample([1 2 3 4], 3)        -> [1 0 0 2 0 0 3 0 0 4 0 0]
        upsample([1 2 3 4], 3, 1)     -> [0 1 0 0 2 0 0 3 0 0 4 0]

    Data layout (matches the AUDIOLIB convention, see AUDIOLIB_gainNCh):
      - planar / non-interleaved (isInterleave=0): in0 is (inChannels, inSamples);
        the sample axis is axis 1.  Output is (inChannels, upFactor*inSamples).
      - interleaved             (isInterleave=1): in0 is (inSamples, inChannels);
        the sample axis is axis 0.  Output is (upFactor*inSamples, inChannels).

    Zeros are inserted along the sample axis in both layouts; channels are
    upsampled independently and the input samples land at sample indices
    phase, phase+upFactor, phase+2*upFactor, ...
    """

    def __init__(self, dType=None, inChannels=1, inSamples=1, upFactor=2, phase=0, isInterleave=0):
        """Configure the upsampler.

        Args:
            dType (str): Data type tag ('iXX', 'uXX', 'f32', 'f64' ...).
            inChannels (int): Number of input channels.
            inSamples (int): Number of input samples per channel.
            upFactor (int): Upsampling factor n (>= 1).
            phase (int): Sample offset, 0 <= phase < upFactor (MATLAB's 3rd arg).
            isInterleave (int): 0 = planar (channels along axis 0),
                                1 = interleaved (channels along axis 1).
        """
        if upFactor < 1:
            raise ValueError("upFactor must be >= 1 (got %d)" % upFactor)
        if not (0 <= phase < upFactor):
            raise ValueError(
                "phase must satisfy 0 <= phase < upFactor (got phase=%d, upFactor=%d)"
                % (phase, upFactor)
            )

        self._dType = dType
        self._inChannels = inChannels
        self._inSamples = inSamples
        self._upFactor = upFactor
        self._phase = phase
        self._isInterleave = isInterleave

    def outSamples(self):
        """Number of output samples per channel = upFactor * inSamples."""
        return self._upFactor * self._inSamples

    def exec(self, in0):
        """Execute the upsample (zero-insertion) algorithm.

        Args:
            in0 (numpy.ndarray): input in memory layout --
                (inChannels, inSamples) if planar, else (inSamples, inChannels).

        Returns:
            numpy.ndarray: zero-inserted output in the same layout convention,
            same dtype as in0.
        """
        in0 = np.asarray(in0)

        if self._isInterleave:
            # interleaved: rows are time-samples, columns are channels.
            # Zero-insert along the row (sample) axis -> axis 0.
            inSamples, inChannels = in0.shape
            out = np.zeros((self._upFactor * inSamples, inChannels), dtype=in0.dtype)
            out[self._phase :: self._upFactor, :] = in0
        else:
            # planar: rows are channels, columns are samples.
            # Zero-insert along the column (sample) axis -> axis 1.
            inChannels, inSamples = in0.shape
            out = np.zeros((inChannels, self._upFactor * inSamples), dtype=in0.dtype)
            out[:, self._phase :: self._upFactor] = in0

        return out


if __name__ == "__main__":
    # --- planar, against the MATLAB doc examples ---
    x = np.array([[1, 2, 3, 4]])
    y0 = AUDIOLIB_upSample(inChannels=1, inSamples=4, upFactor=3, phase=0).exec(x)
    y1 = AUDIOLIB_upSample(inChannels=1, inSamples=4, upFactor=3, phase=1).exec(x)
    print("in            :", x.ravel().tolist())
    print("upsample(x,3) :", y0.ravel().tolist())
    print("upsample(x,3,1):", y1.ravel().tolist())
    assert y0.ravel().tolist() == [1, 0, 0, 2, 0, 0, 3, 0, 0, 4, 0, 0]
    assert y1.ravel().tolist() == [0, 1, 0, 0, 2, 0, 0, 3, 0, 0, 4, 0]
    print("planar OK")

    # --- double (float64) preserves dtype ---
    xd = np.array([[1.5, -2.25, 3.0]], dtype=np.float64)
    yd = AUDIOLIB_upSample(dType="d", inChannels=1, inSamples=3, upFactor=2, phase=1).exec(xd)
    print("upsample(xd,2,1):", yd.ravel().tolist())
    assert yd.dtype == np.float64
    assert yd.ravel().tolist() == [0.0, 1.5, 0.0, -2.25, 0.0, 3.0]
    print("double OK")

    # --- interleaved: (inSamples, inChannels), zero-insert along the row axis ---
    xi = np.array([[1, 2], [3, 4], [5, 6]])  # 3 samples, 2 channels
    yi = AUDIOLIB_upSample(inChannels=2, inSamples=3, upFactor=2, phase=0, isInterleave=1).exec(xi)
    print("interleaved in :", xi.tolist())
    print("interleaved out:", yi.tolist())
    assert yi.tolist() == [[1, 2], [0, 0], [3, 4], [0, 0], [5, 6], [0, 0]]
    # phase=1 shifts the kept samples down one row
    yi1 = AUDIOLIB_upSample(inChannels=2, inSamples=2, upFactor=3, phase=1, isInterleave=1).exec(
        np.array([[1, 2], [3, 4]])
    )
    assert yi1.tolist() == [[0, 0], [1, 2], [0, 0], [0, 0], [3, 4], [0, 0]]
    print("interleaved OK")
