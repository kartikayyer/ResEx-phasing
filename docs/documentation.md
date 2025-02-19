---
layout: default
---

<script type="text/javascript" async src="https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/MathJax.js?config=TeX-AMS_CHTML"></script>

# Documentation [Under construction]

Welcome to the documentation page for `ResEx Phasing`. Here you will find algorithmic details of what the reconstruction program does, descriptions of all the options you can pass in the configuration file, as well as a list of the various utility scripts.

We recommend the user goes through the [basic tutorial](sim-tutorial.html) first in order to get a feel for 'what' the program does. This page discusses more the 'how' of the program.

## Table of Contents

 * [Algorithmic details](#algorithm-details)
   * [Two data sets](#two-data-sets)
   * [Basic Projections](#basic-projections)
   * [Histogram matching](#histogram-matching)
   * [Dynamic support update (solvent flattening)](#dynamic-support)
   * [Spherically symmetric background fitting](#background-fitting)
   * [Composing the projections](#composing-the-projections)
 * [Configuration options](#configuration-options)
   * [Parameters](#parameters)
   * [Files](#files)
   * [Algorithm](#algorithm)
 * [Utilities](#utilities)

## Algorithm Details

The reconstruction is done by a family of phase retrieval methods known as iterative projection algorithms. We start with a random initial condition and incrementally improve it by applying some mapping to it. The "projection" part means that each mapping is composed of projection operations. A projection to a set with a given input returns the member of the set closest to the input, where 'closest' is defined under some metric (usually Euclidean).

In conventional phase retrieval performed in coherent diffractive imaging (CDI), two projections are one which makes the model consistent with the data and the other makes it consistent with a support constraint which says that the density must be non-zero only in a small region. These two projection operations are then composed together in special ways to improve convergence properties.

In our experiment, we have conventional Bragg data at low resolution and twinned continuous diffraction beyond that. To start with, the two data sets are treated independently, with the Bragg diffraction phased using conventional methods like molecular replacement, and the diffuse data is untwinned and phased to produce a higher resolution model.

### Two data sets

The program takes the approach that the user has two sources of data. First some low resolution electron densities obtained through molecular replacement and possibly refinement of Bragg peak data. the other source of data is a 3D volume of intensities obtained by merging full patterns in reciprocal space. The goal is to take these two inputs and combine them to produce a higher resolution electron density by phasing the diffuse intensities in the latter and using the low resolution information from the former.

The phasing part is described in detail in the next sub-sections, but before that, the two data sets must be made consistent. The low-resolution input starts as a user-supplied CCP4/MRC map file. This file is remapped into a voxel grid consistent with that of the diffuse intensity volume, the complex Fourier transform is saved for use in the phase retrieval and a support mask is calculated by blurring and thresholding this model. A cutoff radius is chosen beyond which the low-resolution data is untrustworthy. Similarly a high-resolution cutoff can also be chosen for the diffuse intensities, to avoid merging artifacts at the edges. Finally, a relative scale factor is calculated between the two volumes using voxels in a narrow annulus at medium resolution where both data sets have meaningful values.

At the end, the following objects are produced as input to the phase retrieval routine:
 - Diffuse intensity 3D volume, with values in an annulus between two cutoff radii.
 - Complex 3D volume, with same voxel size as the diffuse intensities, containing the Fourier transform of the low resolution model
 - Support mask, calculated from the low resolution model, again with same voxel size as above.
 - Scale factor relating the Fourier magnitudes of the two volumes in the overlap region.

### Basic Projections

In conventional iterative phase retrieval, one projection is to a modulus constraint by which the magnitude of the Fourier transform is set to be equal to the measured magnitude, while the phase is left unchanged. In this case, since low resolution phases are available, the projection is modified such that in the low resolution range, the amplitudes are set exactly to the input `bragg` amplitudes (both magnitude and phase). Another modification is the inclusion of the effect of point-group symmetrization. The measured intensities are incoherently symmetrized by the point group (incoherent here means that the symmetrization occurs after calculation of the absolute value squared). The projection operation keeps the phases as before, but rescales the magnitudes by the square root of the ratio of the measured to the calculated intensities.

First, the calculated intensity from the current guess is generated by averaging over rotated copies of the Fourier intensities:

$$I_{\mathrm{calc}}(\mathbf{q}) = \frac{1}{N_i}\sum_{i=1}^{N_i} \left|\mathcal{F}(\mathbf{x})\right|^2(\mathbf{R_i . q})$$

where $$\mathcal{F}$$ refers to the discrete Fourier transform operator and $$\mathbf{R}_i$$ represent the rotation operations for the members of the point group. The projection involves rescaling the Fourier transform magnitudes as described above and then inverse Fourier transforming back:

$$P_F(\mathbf{x}) = \mathcal{F}^{-1}\left[\sqrt{\frac{I_{\mathrm{meas}}(\mathbf{q})}{I_{\mathrm{calc}}(\mathbf{q})}}\mathcal{F}(\mathbf{x})\right]$$

The other projection is the standard support projection, which zeroes out everything outside a support mask in real space and leaves all voxels inside the mask unchanged. This can be combined with other projections in real or 'direct' space and so this is termed the direct-space projection, $$P_D(\mathbf{x})$$.

### Dynamic Support

By default, the support mask generated from the input low-resolution model is kept the same throughout the process. However, in order to get the best results, a tight mask should be used and this mask will depend on the higher-resolution modifications to the electron densities.

A procedure similar to solvent flattening in crystallography can be used as an option to update the support mask every iteraton. A box kernel is first used to smoothen the electron density, and then the highest $$N$$ voxels are kept inside the support mask, where $$N$$ is currently the same as the initial number of voxels. Extensions to do a gradually tightening support and to use other smoothing kernels can be implemented.

### Histogram Matching

Another standard density modification tool is histogram matching. This is also provided as an optional method to improve the electron density. Given a histogram of electron density values, one can write a projection which modifies the values inside the support such that their distribution equals the given histogram. This can be done by sorting the values and setting them equal to the nearest value in the inverse CDF (cumulative distribution function) of the target histogram. 

When applied correctly, this technique significantly improves reconstruction quality with noisy data. However, the big caveat is that one must have a good input histogram, which depends both on the voxel size as well as the atomic B-factors. For conventional crystallographic density modification, the resolution and the B factors are closely related, but in this case, one chas to be careful how it is calculated. Another complication is the presence of metal atoms in many proteins which show such rigid-body-like diffuse scattering.

### Background fitting

There is always going to be some amount of structureless background in crystallographic data, both from the solvent in and around the crystal, but also from uncorrelated atomic disorder. This background is usually subtracted from each pattern before being merged into reciprocal space. However, you may often find intensity merges with over- or under-subtracted background. If we can assume that the remainder is spherically symmetric, this can beaccounted for as part of the phase retrieval process. The first step is to add a constant to the input intensities such that they are all non-negative.

The iterate is expanded to include a background magnitude $$B(\mathbf{q})$$ whose square represents the background intensities. The calculated intensity $$I_\mathrm{calc}(\mathbf{q})$$ is then the sum of the Fourier transform term described above and the background intensity at that voxel. The Fourier space projection then rescales both parts of the iterate by the ratio of the measured to the calculated intensities as before. The direct space projection modifies the background part of the iterate by spherically symmetrizing it.

### Composing the projections
The reconstruction algorithm works by composing the projections together in certain specific ways in order to update the current iterate. They are expressed in terms of a mapping by which the next iterate $$x_{n+1}$$ is calculated from the current iterate $$x_n$$ and its two projections $$P_D(x_n)$$ and $$P_F(x_n)$$. The following update rules are implemented:

 * `ER` (Error reduction or alternating projections): The simplest update rule, which takes you to the nearest local minima in the error.
   * <span>$$x_{n+1} = P_D[P_F(x_n)]$$</span>
 * `HIO` (Hybrid Input-Output): Relaxation of error reduction to only partially enforce the support constraint. Here it is expressed in the projection mapping form and extended to more general direct-space projections.
   * <span>$$x_{n+1} = x_n + \beta \left\{P_D\left[\left(1+\frac{1}{\beta}\right) P_F(x_n) - \frac{x_n}{\beta}\right] - P_F(x_n)\right\}$$</span>
 * `DM` (Difference map): Update rule avoids local minima and efficiently searches solution space. Sometimes has trouble with jumping around too much with noisy data.
   * <span>$$x_{n+1} = x_n + \beta \left\{P_D\left[\left(1+\frac{1}{\beta}\right)P_F(x_n) - \frac{x_n}{\beta}\right] - P_F\left[\left(1-\frac{1}{\beta}\right)P_D(x_n) + \frac{x_n}{\beta}\right]\right\}$$</span>
 * `mod-DM` (Modified difference map): Modification of difference map to stay closer to data projection of current iterate.
   * <span>$$x_n' = \beta x_n + (1-\beta) P_F(x_n)$$</span>
   * <span>$$x_{n+1} = x_n' + P_F\left[2 P_D(x_n') - x_n'\right] - P_D(x_n')$$</span>
 * `RAAR` (Relaxed averaged alternating reflections): Another modification of difference map to be more noise-tolerant, originally for the case where the direct space constraint is support plus positivity.
   * <span>$$x_{n+1} = \beta \left\{x_n + P_D\left[2 P_F(x_n)\right] + P_D\left[-x_n\right] - P_F(x_n)\right\} + (1-\beta) P_F(x_n)$$</span>
   * If $$P_D$$ can be assumed to be linear, then the update rule simplifies to
     * <span>$$x_{n+1} = \beta \left\{x_n + P_D\left[2 P_F(x_n) - x_n\right] - P_F(x_n)\right\} + (1-\beta) P_F(x_n)$$</span>

All the update rules except `ER` are equivalent if $$\beta = 1$$. 

## Configuration options

The implementation of the various algorithmic details are implemented via the  configuration file. Here all the possible options are listed and described. The file is split into three sections, `[parameters]`, `[files]` and `[algorithm]`. The first two set up the system and the third describes what the program does.

### [parameters]

| `size` | Edge length of the voxel grid. Currently, only cubic grids are supported |
| `bragg_qmax` | Fraction of edge resolution to which the low-Q Bragg data should be used. If the edge resolution is 2 &#8491; and the Bragg data is trustworthy up to 5 &#8491;, the value will be 2/5 = 0.4 |
| `scale_factor` | Relative scaling between the Bragg and diffuse Fourier magnitudes. Conventionally calculated using the `calc_scale` utility. |
| `num_threads` | Number of threads to use in the multi-threaded FFT and other parallelizable sub-routines. |
| `point_group` | Point group of the diffuse intensities. Currently supported options are `1`, `222` and `4` |

### [files]
The following tokens are all paths to various files. Parentheses indicate optional values.

| `intens_fname` | Diffuse intensity volume. Raw `size`<sup>3</sup> grid of float32 values. |
| `bragg_fname` | Complex valued Fourier amplitudes of low-resolution model. Raw `size`<sup>3</sup> grid of complex64 values. <br/>Note that only values in a central sphere of radius `floor(bragg_qmax * size / 2)` need to be meaningful. |
| `support_fname` | Support mask defining region of real-space where the density can be non-zero. Raw `size`<sup>3</sup> grid of uint8 values. |
| `output_prefix` | Prefix to output files. This is usually a path plus the initial part of a filename. For example, the log file will be at `<output_prefix>-log.dat` |
| (`input_fname`) | Optional path to initial guess for electron densities. If you want to continue a previous reconstruction pass in the `<output_prefix>-last.raw` file. Raw `size`<sup>3</sup> grid of float32 values. |
| (`inputbg_fname`) | Optional path to initial guess for background intensities. Only used if the `bg_fitting` option is turned on. Raw `size`<sup>3</sup> grid of float32 values. |

### [algorithm]
As before parentheses indicate optional values. Flags can take the value `0` for off/false and `1` for on/true. All flags are off by default.

| `algorithm` | Space-separated list of alternating numbers and algorithm codes. Code options are `ER`, `DM`, `HIO`, `RAAR`, `mod-DM`. See [description](#composing-the-projections) above for details of what they do. For example `100 DM 50 ER 100 DM` will do 100 iterations of difference map followed by 50 iterations of error reduction, followed by 100 more iterations of difference map. |
| `avg_algorithm` | Same format as in the `algorithm` option, but a running average of the projections is calculated over these iterations. |
| `beta` | Relaxation parameter for all the algorithms above except `ER`. Note that for `beta = 1`, all algorithms except `ER` are equivalent. |
| (`bg_fitting`) | FLAG for whether to retrieve a spherically symmetric background in addition to the real-space model. See [description](#background-fitting) above for details. |
| (`local_variation`) | FLAG for whether to dynamically update the support using a local variation threshold *a la* solvent flattening. If `0`, the support stays constant. |
| (`positivity`) | FLAG for whether to assume the electron densities in the support are non-negative. Usually not applicable since the zero value in protein crystallography is set to be the solvent level. |
| (`normalize_prtf`) | FLAG for whether to sharpen the output model by the reciprocal of the PRTF. The averaging process by which the PRTF is calculated reduces the fourier magnitudes. If this option is turned on, the output reconstruction is high-pass filtered to cancel out this effect. This makes the I vs q dependence match the diffuse intensities, which may be more suitable for refinement. However, it also enhances noise, so the right resolution cutoff must be chosen. |
| (`histogram`) | FLAG for whether to apply a histogram projection. Requires `hist_fname` to be set. See [description](#histogram-matching) above for details. |
| (`hist_fname`) | Path to two-column text file specifying the histogram to be matched against. The first line of the file is the number of rows. Following that, the first column has the value of the density in the center of the bin and the second column has the fraction of voxels with that value. |
| (`blurring`) | FLAG for whether to rotationally blur the calculated Fourier intensities before comparing with the data. Needs either `quat_fname` or both `num_div` and `sigma_deg`.
| (`quat_fname`) | Path to quaternion file in the [*Dragonfly*](http://github.com/duaneloh/Dragonfly/wiki) format. |
| (`num_div`) | Refinement level of quasi-uniform SO(3) rotation sampling. Number of samples of the whole rotation group is `10*(5*num_div^2 + num_div)`. Needs `sigma_deg` to also be specified. |
| (`sigma_deg`) | Width in degrees of Gaussian distribution of angles centered on the identity quaternion. Combined with `num_div` this generates a quaternion list, which can be used for rotational blurring |

## Utilities

In addition to the main reconstruction code, quite a number of helper utilities are provided. They can be used to prepare or modify the input data or analyze the reconstruction output. Additionally, there are some convenience scripts which employ one or more utilities.

The following utilities are compiled into the `utils` directory. To get usage information, run
```
$ ./utils/<util_name>
``` 

| `band_limit` | Smoothens intensity file using support mask
| `bin_intens` | Downsample given volume by integer factor
| `boost_cont` | Top hat high pass filter electron density model
| `calc_cc` | Calculates Pearson CC coefficient as a function of radius
| `calc_fsc` | Calculates Fourier Shell Correlation between two densities
| `calc_scale` | Calculates scale factor between two intensities
| `calc_sfac` | Calculates structure factors using French Wilson procedure
| `calc_smoothness` | Effective tangential smoothness of merge as a function of radius
| `calc_snr` | Calculate SNR metrics for intensity volume
| `combine` | Average a set of reconstructions in the results folder
| `create_support` | Create support mask from density
| `fstretch` | Slightly change q-sampling for given model
| `gen_dens` | Inverse fourier transform complex amplitudes to get density
| `gen_fdens` | Fourier transform electron densities
| `gen_map` | Produce CCP4/MRC map from electron density
| `liquidize` | Apply liquid-like motion blurring to Fourier amplitudes
| `merge_synch` | ./utils/merge_synch <file_list>
| `poisson` | Poisson sample intensities according to given mean at edge
| `rad_avg` | Calculate radial average of intensities
| `read_map` | Parse CCP4/MRC map with given target voxel resolution
| `scale_q` | Q-dependent scale factors between two intensities
| `sharpen` | High pass filter by negative B-factor
| `zero_outer` | Process outer and inner parts of intensity file
