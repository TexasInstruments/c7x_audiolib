:icon: material/home

AUDIOLIB — C7x Audio Processing Library
========================================

AUDIOLIB is TI's optimized audio processing library for the **C7™** DSP architecture.
It provides a suite of kernels targeting the C7™ vector and streaming engines (SE/SA),
enabling high-throughput audio signal processing on TI SoCs such as AM62D and AM275.
The Sample Rate Converter (``AUDIOLIB_ssrc``) additionally leverages the Matrix Multiply
Accelerator (MMA) for maximum throughput.

Each kernel ships in two variants selected at init time: a C-native reference (``_cn``,
portable and functionally correct) and a C7™ intrinsic-optimized variant (``_ci``,
hardware-optimized for SE/SA streaming engines).

.. grid:: 1 2 2 3
   :gutter: 3
   :class-container: ti-home-grid

   .. grid-item-card:: :si-icon:`material/rocket-launch` Getting Started
      :link: getting_started
      :link-type: doc

      Build, run, and generate test data — from PC host emulation to on-target EVM.

   .. grid-item-card:: :si-icon:`material/book-open-variant` API Reference
      :link: api/index
      :link-type: doc

      Every kernel, grouped by category, with signatures, parameters, and usage notes.

   .. grid-item-card:: :si-icon:`material/chart-line` Performance
      :link: perf/index
      :link-type: doc

      Measured kernel cycle counts on C7504 (AM62D) and C7524 (AM275).

   .. grid-item-card:: :si-icon:`material/file-tree` Repository Layout
      :link: repository_layout
      :link-type: doc

      A guided tour of the source tree — kernels, tests, and build files.

   .. grid-item-card:: :si-icon:`material/scale-balance` License
      :link: license
      :link-type: doc

      AUDIOLIB is distributed under the Apache License 2.0.

.. toctree::
   :hidden:
   :maxdepth: 2

   getting_started
   api/index
   perf/index
   repository_layout
   license
