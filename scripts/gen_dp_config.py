#!/usr/bin/python3

import sys
import math

HELP_FLAGS = { '-h': True, '--help': True }

def inverse_laplace_cdf(span, prob):
  sgn = 0
  if prob < 0.5:
    sgn = -1
  if prob > 0.5:
    sgn = 1

  return 0 - span * sgn * math.log(1 - 2 * abs(prob - 0.5))

def laplace_cdf(span, x):
  sgn = 0
  if x < 0:
    sgn = -1
  if x > 0:
    sgn = 1

  return 0.5 + 0.5 * sgn * (1 - math.exp(-1 * abs(x) / span))

if __name__ == "__main__":
  # Print help message.
  need_help = len(sys.argv) < 2
  for arg in sys.argv:
    if HELP_FLAGS.get(arg, False):
      need_help = True
      break

  if need_help:
    print("Usage: python3 gen_dp_config.py <epsilon> <delta>")
    sys.exit(0)

  # Read command line arguments.
  epsilon = float(sys.argv[1])
  delta = float(sys.argv[2])

  # Print parameters.
  span = 1.0 / epsilon
  cutoff = inverse_laplace_cdf(span, delta / 2.0)
  if cutoff >= 0:
    print('Cannot positive cutoff ', cutoff)
    sys.exit(1)

  cutoff = abs(cutoff)
  print('span =', span)
  print('cutoff =', cutoff)
  print('probability of having noise > 10 = ', 1 - laplace_cdf(span, 10 - cutoff))
  print('probability of having noise > 25 = ', 1 - laplace_cdf(span, 25 - cutoff))
  print('probability of having noise > 50 = ', 1 - laplace_cdf(span, 50 - cutoff))
  print('probability of having noise > 100 = ', 1 - laplace_cdf(span, 100 - cutoff))
  print('probability of having noise > 150 = ', 1 - laplace_cdf(span, 150 - cutoff))
