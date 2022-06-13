import sys
import json
import math
import re

import matplotlib
import matplotlib.pyplot
import numpy as np

from os import path, listdir
from tabulate import tabulate

DESCRIPTION_DIR = 'descriptions'
OUTPUT_DIR = path.join('..', 'orchestrator', 'outputs')
FIGS_DIR = 'figs'

DPPIR_X_MAP = {
  'queries': 4,
  'tableSize': 1,
  'parties': 2,
  'parallelism': 3
}
OTHERS_X_MAP = {
  'tableSize': 1,
  'queries': 2
}

MEAN_NOISE = 276

DPPIR_ONLINE_REGEX = r'Online time: ([0-9]+)ms'
DPPIR_OFFLINE_REGEX = r'Offline time: ([0-9]+)ms'
CHECKLIST_REGEX = r'Server [0-9] time: ([0-9]+m)?([0-9.]+)([a-z]+) and '
SEALPIR_REGEX = r'Server time: ([0-9]+)ms'

class Table:
  def __init__(self, description):
    self.experiment_name = description['experiment-name']
    self.rows_attr = description['table']['rows-attr']
    self.cols_attr = description['table']['cols-attr']
    self.rows = list(description['table']['rows'])
    self.cols = list(description['table']['cols'])

    self.headers = list(description['labels']['headers'])
    self.subheaders = self.cols

    self.table = []
    for i in range(len(self.rows)):
      self.table.append([self.rows[i]] + [None for i in self.cols])

  def to_screen(self):
    print(tabulate(self.table, headers=[self.headers[0]] + self.subheaders))

  def to_latex(self):
    begin = "\\begin{tabular}{c|c|c}"
    headers = "\\multirow{2}{*}{\\textbf{" + self.headers[0] + "}} & \\multicolumn{2}{c}{\\textbf{" + self.headers[1] + "}} \\\\"
    subheaders = "&   \\textbf{\quad " + self.subheaders[0] + "\quad}   & \\textbf{" + self.subheaders[1] + "} \\\\ \hline"
    rows = []
    for row in self.table:
      rows.append(' & '.join([str(r) for r in row]))
    rows = ' \\\\\n'.join(rows)
    end = "\\end{tabular}%"
    return '{}\n{}\n{}\n{}\n{}'.format(begin, headers, subheaders, rows, end)

  def compatible_directory(self, dirname):
    if not dirname.startswith('{}-'.format(self.experiment_name)):
      return False
    # tables only supported for DP-PIR.
    return len([c for c in dirname if c == '-']) == 6

  def compatible_file(self, filename):
    return filename.startswith('party-0')

  def parse_data(self, dirname, filename, data):
    parts = dirname.split('-') + filename.split('-')
    x = int(parts[DPPIR_X_MAP[self.rows_attr]])
    assert self.cols_attr == 'mode', 'cols-attr not supported {}/{}'.format(dirname, filename)

    offline_matches = re.findall(DPPIR_OFFLINE_REGEX, data)
    online_matches = re.findall(DPPIR_ONLINE_REGEX, data)
    assert len(offline_matches) == 1, 'Bad regex DPPIR {}/{}'.format(dirname, filename)
    assert len(online_matches) == 1, 'Bad regex DPPIR {}/{}'.format(dirname, filename)

    offline = float(offline_matches[0]) / 1000.0
    online = float(online_matches[0]) / 1000.0
    self.table[self.rows.index(x)][self.cols.index('Offline') + 1] = offline
    self.table[self.rows.index(x)][self.cols.index('Online') + 1] = online

class Series:
  def __init__(self, protocol, mode, description):
    self.protocol = protocol
    self.mode = mode
    self.experiment_name = description['experiment-name']
    self.xaxis = description['axis']['x']
    self.Xs = list(description['x'])
    self.Ys = [None for i in self.Xs]
    self.interpolate_xs = set(description['interpolate']['{}-{}'.format(protocol, mode)])
    self.table_size = description['tableSize']
    self.queries = description['queries']
    if self.protocol == 'DP-PIR':
      assert mode in ('Online', 'Offline'), 'bad mode'
      self.label = (protocol, mode)[description['labels']['graphs']]
    elif self.protocol == 'Checklist':
      assert mode in ('DPF', 'Punc'), 'bad mode'
      self.label = 'Checklist' if mode == 'Punc' else 'DPF'
    elif self.protocol == 'SealPIR':
      self.label = 'SealPIR'
      assert mode in ('SealPIR'), 'bad mode'
    else:
      assert False, 'bad protocol'

  def compatible_directory(self, dirname):
    if not dirname.startswith('{}-'.format(self.experiment_name)):
      return False

    if self.protocol == 'DP-PIR':
      return len([c for c in dirname if c == '-']) == 6
    else:
      return len([c for c in dirname if c == '-']) == 2

  def compatible_file(self, filename):
    if self.protocol == 'DP-PIR': return filename.startswith('party-0')
    elif self.protocol == 'Checklist': return filename == 'client.log'
    elif self.protocol == 'SealPIR': return filename == 'sealpir.log'

  def extract_x(self, dirname, filename):
    parts = dirname.split('-') + filename.split('-')
    if self.protocol == 'DP-PIR':
      return int(parts[DPPIR_X_MAP[self.xaxis]])
    else:
      return int(parts[OTHERS_X_MAP[self.xaxis]])

  def extract_y(self, dirname, filename, data):
    if self.protocol == 'DP-PIR':
      matches = []
      if self.mode == 'Online':
        matches = re.findall(DPPIR_ONLINE_REGEX, data)
      elif self.mode == 'Offline':
        matches = re.findall(DPPIR_OFFLINE_REGEX, data)
        if len(matches) == 0:
          return None
      assert len(matches) == 1, 'Bad regex DPPIR {}/{}'.format(dirname, filename)
      return float(matches[0]) / 1000.0

    if self.protocol == 'Checklist':
      y = -100000
      for m, t, u in re.findall(CHECKLIST_REGEX, data):
        if len(m) > 0:
          m = int(m[:-1]) * 60
        else:
          m = 0

        t = float(t)
        if u == 's':
          y = max(y, m + t)
        elif u == 'ms':
          y = max(y, m + (t / 1000.0))
        else:
          assert False, 'invalid unit'
      return y

    if self.protocol == 'SealPIR':
      matches = re.findall(SEALPIR_REGEX, data)
      assert len(matches) == 1, 'Bad regex SealPIR {}/{}'.format(dirname, filename)
      return float(matches[0]) / 1000.0

    assert False, 'Unreachable'

  def parse_data(self, dirname, filename, data):
    x = self.extract_x(dirname, filename)
    y = self.extract_y(dirname, filename, data)
    self.Ys[self.Xs.index(x)] = y

  def interpolate(self, x):
    pairs = [ (x, y) for x, y in zip(self.Xs, self.Ys) if not y is None ]
    # Interpolate with # queries.
    if self.xaxis == 'queries':
      if self.protocol == 'DP-PIR':
        x1, y1 = pairs[-2]
        x2, y2 = pairs[-1]
        # 2 equations 2 unknowns.
        query_unit = (y2 - y1) / (x2 - x1)
        noise_cost = y1 - (query_unit * x1)
        # Interpolate.
        return noise_cost + (x * query_unit)
      else:
        maxX, maxY = pairs[-1]
        return (x * maxY) / maxX
    elif self.xaxis == 'tableSize':
      if self.protocol == 'DP-PIR':
        x1, y1 = pairs[-2]
        x2, y2 = pairs[-1]
        # Total number of noise queries.
        n1 = x1 * MEAN_NOISE
        n2 = x2 * MEAN_NOISE
        # 2 equations 2 unknowns.
        one_noise_cost = (y2 - y1) / (n2 - n1)
        queries_cost = y2 - (one_noise_cost * n2)
        # Interpolate.
        return one_noise_cost * (x * MEAN_NOISE) + queries_cost

    assert False, 'bad interpolate!'

  def interpolate_all(self):
    for x, y in zip(self.Xs, self.Ys):
      if not y is None: continue
      assert x in self.interpolate_xs, 'Missing value for {}'.format(x)
      self.Ys[self.Xs.index(x)] = self.interpolate(x)

  def get_label(self):
    return self.label

class ExtrapolateData:
  def __init__(self, protocol, mode, description):
    self.protocol = protocol
    self.mode = mode
    self.experiment_name = description['experiment-name']
    self.xaxis = description['axis']['x']
    assert self.xaxis == 'tableSize', 'bad xaxis'
    self.Xs = list(description['x'])
    self.costs = [None for x in self.Xs]
    if self.protocol == 'DP-PIR':
      self.costs_tmp = (None, None)
    if self.protocol == 'DP-PIR':
      assert mode in ('Online'), 'bad mode'
    elif self.protocol == 'Checklist':
      assert mode in ('DPF', 'Punc'), 'bad mode'
    elif self.protocol == 'SealPIR':
      assert mode in ('SealPIR'), 'bad mode'
    else:
      assert False, 'bad protocol'

  def compatible_directory(self, dirname):
    if not dirname.startswith('{}-'.format(self.experiment_name)):
      return False

    if self.protocol == 'DP-PIR':
      return len([c for c in dirname if c == '-']) == 6
    else:
      return len([c for c in dirname if c == '-']) == 2

  def compatible_file(self, filename):
    if self.protocol == 'DP-PIR': return filename.startswith('party-0')
    elif self.protocol == 'Checklist': return filename == 'client.log'
    elif self.protocol == 'SealPIR': return filename == 'sealpir.log'

  def extract_x(self, dirname, filename):
    parts = dirname.split('-') + filename.split('-')
    if self.protocol == 'DP-PIR':
      return int(parts[DPPIR_X_MAP[self.xaxis]])
    else:
      return int(parts[OTHERS_X_MAP[self.xaxis]])

  def extract_q(self, dirname, filename):
    parts = dirname.split('-') + filename.split('-')
    if self.protocol == 'DP-PIR':
      return int(parts[DPPIR_X_MAP['queries']])
    else:
      return int(parts[OTHERS_X_MAP['queries']])

  def extract_y(self, dirname, filename, data):
    if self.protocol == 'DP-PIR':
      matches = []
      if self.mode == 'Online':
        matches = re.findall(DPPIR_ONLINE_REGEX, data)
      assert len(matches) == 1, 'Bad regex DPPIR {}/{}'.format(dirname, filename)
      return float(matches[0]) / 1000.0

    if self.protocol == 'Checklist':
      y = -100000
      for m, t, u in re.findall(CHECKLIST_REGEX, data):
        if len(m) > 0:
          m = int(m[:-1]) * 60
        else:
          m = 0

        t = float(t)
        if u == 's':
          y = max(y, m + t)
        elif u == 'ms':
          y = max(y, m + (t / 1000.0))
        else:
          assert False, 'invalid unit'
      assert y != -100000, 'Could not parse Checklist'
      return y

    if self.protocol == 'SealPIR':
      matches = re.findall(SEALPIR_REGEX, data)
      assert len(matches) == 1, 'Bad regex SealPIR {}/{}'.format(dirname, filename)
      return float(matches[0]) / 1000.0

    assert False, 'Unreachable'

  def compute_cost(self, x, y, q):
    # find per query cost for this table size (x).
    if self.protocol == 'DP-PIR':
      noise, real = self.costs_tmp
      if q == 1:  # compute noise cost
        noise = float(y) / (x * MEAN_NOISE)
      else:
        real = float(y) / q
      self.costs_tmp = (noise, real)

    else:
      self.costs[self.Xs.index(x)] = y / float(q)

  def parse_data(self, dirname, filename, data):
    x = self.extract_x(dirname, filename)
    q = self.extract_q(dirname, filename)
    y = self.extract_y(dirname, filename, data)
    self.compute_cost(x, y, q)

  def interpolate_all(self):
    if self.protocol == 'DP-PIR':
      self.costs = [self.costs_tmp for x in self.Xs]

    else:
      pairs = [ (x, cost) for x, cost in zip(self.Xs, self.costs) if not cost is None ]
      for x, y in zip(self.Xs, self.costs):
        if y is None:
          assert self.mode == 'Punc', 'bad mode interpolate'
          x1, y1 = pairs[-1]
          self.costs[self.Xs.index(x)] = y1 * math.sqrt(x / x1)

def read_description(name):
  fpath = path.join(DESCRIPTION_DIR, '{}.json'.format(name))
  with open(fpath) as f:
    return json.load(f)

def read_plot_data(name, description):
  data = {}
  for pair in description['axis']['y']:
    protocol, mode = tuple(pair)
    series = Series(protocol, mode, description)

    for d in listdir(OUTPUT_DIR):
      if not series.compatible_directory(d):
        continue
      for f in listdir(path.join(OUTPUT_DIR, d)):
        if not series.compatible_file(f):
          continue

        fpath = path.join(OUTPUT_DIR, d, f)
        with open(fpath, "r") as datafile:
          series.parse_data(d, f, datafile.read())

    series.interpolate_all()
    data[series.get_label()] = series.Ys

  return data

def read_ratio_data(name, description):
  dppir = ExtrapolateData('DP-PIR', 'Online', description)
  baseline = ExtrapolateData(description['baseline'][0], description['baseline'][1], description)

  for d in listdir(OUTPUT_DIR):
    if dppir.compatible_directory(d):
      for f in listdir(path.join(OUTPUT_DIR, d)):
        if dppir.compatible_file(f):
          fpath = path.join(OUTPUT_DIR, d, f)
          with open(fpath, "r") as datafile:
            dppir.parse_data(d, f, datafile.read())

    if baseline.compatible_directory(d):
      for f in listdir(path.join(OUTPUT_DIR, d)):
        if baseline.compatible_file(f):
          fpath = path.join(OUTPUT_DIR, d, f)
          with open(fpath, "r") as datafile:
            baseline.parse_data(d, f, datafile.read())

  dppir.interpolate_all()
  baseline.interpolate_all()

  print('Database size: ', description['x'])
  data = {}
  for label in description['axis']['y']:
    speedup = float(label[:-1])
    queries_data = []
    data[label] = []
    for idx in range(len(description['x'])):
      tableSize = description['x'][idx]
      # compute ratio such that baseline_runtime = speedup * dppir_runtime
      baseline_cost = baseline.costs[idx]
      noise_cost, real_cost = dppir.costs[idx]
      total_noise = (tableSize * MEAN_NOISE) * noise_cost
      queries = (speedup * total_noise) / (baseline_cost - speedup * real_cost)
      ratio = queries / tableSize
      queries_data.append(queries)
      data[label].append(ratio)
    print(label, ' Speedup at ', queries_data, 'queries!')
    print(label, ' Sppedup at ', data[label], 'ratios!')

  # Compute speedups for Google Play scenario
  if description['experiment-name'] == 'figure2':
    google_play_tablesize = 2500000
    google_play_queries = [3*(10**9), 3*100*(10**9), 3*100*(10**9)]
    noise_means = [276, 276, 276 * 100]
    speedups = []
    for q, n in zip(google_play_queries, noise_means):
      base = baseline.costs[description['x'].index(google_play_tablesize)] * q
      nc, rc = dppir.costs[description['x'].index(google_play_tablesize)]
      us = nc * google_play_tablesize * n + rc * q
      speedups.append(base / us)
    print('Google play store example', speedups)

  return data

def read_table_data(name, description):
  table = Table(description)
  for d in listdir(OUTPUT_DIR):
    if not table.compatible_directory(d):
      continue
    for f in listdir(path.join(OUTPUT_DIR, d)):
      if not table.compatible_file(f):
        continue

      fpath = path.join(OUTPUT_DIR, d, f)
      with open(fpath, "r") as datafile:
        table.parse_data(d, f, datafile.read())

  return table

def simple_plot(pdf, description, data):
  # Setup frame and margins.
  margins = description.get('margins', (0.13, 0.87, 0.15, 0.99))
  matplotlib.rcParams.update({'figure.figsize': description.get('figsize', (7.0, 3.8))})
  fig, ax = matplotlib.pyplot.subplots()
  matplotlib.pyplot.subplots_adjust(left=margins[0], right=margins[1],
                                    bottom=margins[2], top=margins[3])

  # Plot
  signs = ["x:", "p-.", "v--", ".-"]
  X = np.array(description['x'])
  for name in data.keys():
    ax.plot(X, np.array(data[name]), signs.pop(), label=name)

  # Show any vertical lines.
  for v in description.get('vertical', []):
    ax.axvline(x=v, color='k', linestyle='--')

  # Axis
  ax.set_xlabel(description['labels']['x'])
  ax.set_ylabel(description['labels']['y'])
  if description.get('xscale', 'log') == 'log':
    ax.set_xscale('log')
  if description.get('yscale', None):
    ax.set_yscale(description['yscale'])
  matplotlib.pyplot.xlim(description['ranges']['x'])
  matplotlib.pyplot.ylim(description['ranges']['y'])

  # Formatting
  ax.grid()
  ax.legend(ncol=1, loc=description.get('legend', 'best'))

  if pdf:
    filename = '{}.pgf'.format(description['experiment-name'])
    matplotlib.pyplot.savefig(path.join(FIGS_DIR, filename))
  else:
    matplotlib.pyplot.show()

def split_plot(pdf, description, data):
  # Setup frame and margins.
  margins = description.get('margins', (0.13, 0.87, 0.15, 0.99))
  matplotlib.rcParams.update({'figure.figsize': description.get('figsize', (7.0, 3.8))})
  fig, (top_ax, bot_ax) = matplotlib.pyplot.subplots(2, 1, sharex=True, gridspec_kw={'height_ratios': [4, 1]})
  fig.subplots_adjust(hspace=0.08)
  matplotlib.pyplot.subplots_adjust(left=margins[0], right=margins[1],
                                    bottom=margins[2], top=margins[3])

  # Do bottom split then top split
  bot_ax.set_ylim(description['ranges']['y'][0], description['split'][0])
  top_ax.set_ylim(description['split'][1], description['ranges']['y'][1])


  # Plot.
  colors = matplotlib.rcParams["axes.prop_cycle"]()
  signs = ["x:", "p-.", "v--", ".-"]
  X = np.array(description['x'])
  bot_label, bot_data = description["bot"], data[description["bot"]]
  top_label, top_data = description["top"], data[description["top"]]
  top_ax.plot(X, np.array(top_data), signs.pop(), label=top_label, color=next(colors)["color"])
  bot_ax.plot(X, np.array(bot_data), signs.pop(), label=bot_label, color=next(colors)["color"])

  # Axis.
  bot_ax.set_xlabel(description['labels']['x'])
  fig.text(0.03, 0.58, description['labels']['y'], ha='center', va='center', rotation='vertical')
  #top_ax.set_ylabel(description['labels']['y'])

  # Get the broken feel: hide all indications that top and bottom are different subplots.
  top_ax.spines['bottom'].set_visible(False)
  top_ax.axes.xaxis.set_ticks_position('none')
  bot_ax.spines['top'].set_visible(False)

  d = .013  # how big to make the diagonal lines in axes coordinates
  # arguments to pass to plot, just so we don't keep repeating them
  kwargs = dict(transform=top_ax.transAxes, color='k', clip_on=False)
  top_ax.plot((-d, +d), (-d/2, +d/2), **kwargs)        # top-left diagonal
  top_ax.plot((1 - d, 1 + d), (-d/2, +d/2), **kwargs)  # top-right diagonal

  kwargs.update(transform=bot_ax.transAxes)  # switch to the bottom axes
  bot_ax.plot((-d, +d), (1 - 2*d, 1 + 2*d), **kwargs)  # bottom-left diagonal
  bot_ax.plot((1 - d, 1 + d), (1 - 2*d, 1 + 2*d), **kwargs)  # bottom-right diagonal

  # Formatting
  bot_ax.grid()
  top_ax.grid()

  handles, labels = [(a + b) for a, b in zip(top_ax.get_legend_handles_labels(), bot_ax.get_legend_handles_labels())]
  fig.legend(handles, labels, loc=(0.15, 0.72))

  if pdf:
    filename = '{}.pgf'.format(description['experiment-name'])
    matplotlib.pyplot.savefig(path.join(FIGS_DIR, filename))
  else:
    matplotlib.pyplot.show()

if __name__ == '__main__':
  pdf = 'pdf' == (None if len(sys.argv) < 2 else sys.argv[1])
  if pdf:
    matplotlib.use("pgf")
    matplotlib.rcParams.update({
        "pgf.texsystem": "pdflatex",
        'font.family': 'serif',
        'text.usetex': True,
        'pgf.rcfonts': False,
        'legend.loc': 'lower right',
        'font.size': 18,
    })

  files = sys.argv[2:]
  if len(files) == 0:
    files = list([f[:-5] for f in listdir(DESCRIPTION_DIR) if f.endswith('.json')])

  for name in files:
    print('Plotting', name)

    # parse description and data
    description = read_description(name)

    # Plot by type
    if description['type'] == 'simple-plot':
      data = read_plot_data(name, description)
      simple_plot(pdf, description, data)
    if description['type'] == 'ratio-plot':
      data = read_ratio_data(name, description)
      simple_plot(pdf, description, data)
    elif description['type'] == 'split-plot':
      data = read_plot_data(name, description)
      split_plot(pdf, description, data,)
    elif description['type'] == 'table':
      table = read_table_data(name, description)
      if pdf:
        filename = '{}.tex'.format(description['experiment-name'])
        with open(path.join(FIGS_DIR, filename), 'w', encoding='utf-8') as ofile:
          ofile.write(table.to_latex())
      else:
        table.to_screen()

    else:
      assert False, 'Unsupported plot type'

