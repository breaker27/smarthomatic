RGB Dimmer
==========

The RGB dimmer *usage* is described on the smarthomatic homepage. This text
describes primarily the idea behind the *algorithm for the animation* of a
sequence of colors.

If an animation uses the repeat function, a sequence of colors is repeated.
Such a sequence is called a *cycle*. An *animation step* is a fading from
one color to the next.

Initial parameters
------------------

The initial data that determines the behavior of the animation consists of
the following variables:

- repeat: Repeat count (>= 0)
- autoreverse: Autoreverse on/off (boolean)
- anim_col_i: 1 to 10 indexed colors (0..63)
- anim_time: 1 to 10 time values used to animate color x to x + 1

See the homepage about what the values mean.

Internal data structures
------------------------

These are the most important additional internal data structures used for
the animation:

- A struct rgb_color_t is defined which consists of a red, green and blue
  color component (0..255).
- anim_col: An array of 30 rgb_color_t colors.
- anim_time: An array of 30 time values (the first 10 contain the initial
  values).
- rfirst: The index of the first color to animate within one repeat cycle.
- rlast: The index of the last color to animate within one repeat cycle.
- llast: The index of the last color to animate *only* in the last cycle.
  Usually llast > rlast.
- key_idx: One important position to calculate how the color values are
  copied (see initialization).

Animation initialization
------------------------

To make the animation algorithm quicker, easier to understand and maintain,
the initial parameters are transformed into the additional data structures
before the animation starts. After that, the animation itself can be very
simple.

The transformation / initialization of values highly depends on how the
parameters "repeat" and "autoreverse" are set.

The goal is to transform any animation to one which only is played back in
one direction. The several indexes then dictate where the animation has to
jump to another index, forming a repeat cycle. Especially an animation with
active autoreverse function is transformed into a linear one by "unfolding"
the color sequence which would otherwise be played backwards.

See the scanned image "animation_initialization.jpg" for a detailed example.
It describes how initialization is done depending on "repeat" and
"autoreverse".

Animation sequence
------------------

After initialization is done, the animation is pretty simple.

The most important rule in advance:

- When animation is at color position (col_pos) x, it means that color x
  is faded into color x+1 in the current animation step.
- This means the animation ends with color position z-1 when z is the last
  used color in the array.
- The duration for the step x is stored in anim_time[x].

The animation algorithm is as follows:

- Start at col_pos 0 and animate from color 0 to 1 with time 0.
- If end of step reached, go to next col_pos.
- When animation step at rlast is completed (col_pos = rlast) and
  repeat != 1, decrease repeat by 1 (if not 0 already) and jump to
  rfirst.
- When animation step at llast is completed (col_pos = llast) and
  repeat = 1, stop animation.

This especially means that an animation with repeat = 0 is repeated
infinitely.
