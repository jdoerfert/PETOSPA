import os
import re
import sys
import time
import shutil
import random
import filecmp
import tempfile
import collections
import logging as log
import subprocess as sp
from utils import serializable
# from pathlib import Path

temp_directory = os.path.join(tempfile.gettempdir(),
                              'opT' + str(random.randint(0, 99999)))
os.mkdir(temp_directory)

DEBUG_CONSOLE = False
ANNOTATE_SOURCE = True
REPORT_EVERY_NUM_TRIES = 10

# Use None to disable
DEBUG_TIME = '%b %d, %H:%M:%S'
DEBUG_FILE = os.path.join(temp_directory, 'debug.log')
STORE_MISMATCH_PATH = os.path.join(temp_directory, 'io_mismatches.txt')


try:
    import colorlog as logging
    if DEBUG_TIME:
        debug_string = ("%(asctime)s - %(log_color)s%(levelname)-8s%(reset)s "
                        "%(message)s")
    else:
        debug_string = "%(log_color)s%(levelname)-8s%(reset)s %(message)s"
    formatter = logging.ColoredFormatter(debug_string,
                                         reset=True,
                                         datefmt=DEBUG_TIME,
                                         log_colors={
                                             'DEBUG':    'cyan',
                                             'INFO':     'green',
                                             'WARNING':  'yellow',
                                             'ERROR':    'red',
                                             'CRITICAL': 'red,bg_white',
                                         },
                                         secondary_log_colors={},
                                         style='%')
except Exception:
    logging = log
    formatter = log.Formatter('%(asctime)s - %(levelname)-10s - %(message)s',
                              datefmt='%b %d, %H:%M:%S')

logger = logging.getLogger('')
logger.setLevel(log.DEBUG)

ch = log.StreamHandler()
ch.setLevel(log.DEBUG if DEBUG_CONSOLE else log.INFO)
ch.setFormatter(formatter)
logger.addHandler(ch)

# add the handlers to the logger
if DEBUG_FILE:
    # create file handler which logs even debug messages
    fh = log.FileHandler(DEBUG_FILE)
    fh.setLevel(log.DEBUG)
    fh.setFormatter(formatter)
    logger.addHandler(fh)

logger.info(f'Compiled source file version are copied to {temp_directory}')
if DEBUG_FILE:
    logger.info(f'Debug output is logged to {DEBUG_FILE}')
if STORE_MISMATCH_PATH:
    logger.info(f'Mismatch outputs is stored to {STORE_MISMATCH_PATH}')


class OptimisticChoice(object):
    def __init__(self, max_options, opportunity_no, function_no, category,
                 kind, name, function, position):
        self.category = category
        self.kind = kind
        self.opportunity_no = int(opportunity_no)
        self.function_no = int(function_no)
        self.max_options = int(max_options)
        self.name = name
        self.function = function
        self.position = int(position)
        self.fixed = False
        self.value = self.max_options - 1

    def getOptimisticValue(self):
        return self.value

    def setOptimisticValue(self, value):
        assert not self.fixed
        self.value = value

    def fix(self, value):
        assert not self.fixed
        self.fixed = True
        self.value = value


class ChoiceExplorer(object):
    def __init__(self, max_tries, max_time, time_end, num_opportunities,
                 optimistic_choices, initial_control_string):
        assert isinstance(num_opportunities, int)
        assert isinstance(optimistic_choices, collections.Iterable)
        assert len(optimistic_choices) > 0
        assert all([isinstance(oc, OptimisticChoice)
                    for oc in optimistic_choices])

        self.time_end = time_end
        self.max_time = max_time
        self.max_tries = max_tries
        self.num_opportunities = num_opportunities
        self.optimistic_choices = optimistic_choices

        self.finished = False
        self.first_pos = 0 #optimistic_choices[0].position
        self.last_pos = len(optimistic_choices)
        asciistr = '['+(''.join(chr(i) for i in range(ord('0'), ord('~'))))+']'
        self.optimistic_value_re = re.compile(asciistr)

        self.current_oc = self.optimistic_choices[0]
        self.control_string = initial_control_string #'0' * self.num_opportunities
        self.oc_start_position = 0
        self.position_map = {}
        last_function_no = -1
        last_opportunity_no = -1
        for oc in self.optimistic_choices:
            if last_function_no != oc.function_no:
                self.control_string += '#f' + str(oc.function_no) + 'f'
                last_function_no = oc.function_no
            if last_opportunity_no != oc.opportunity_no:
                self.control_string += '#c' + chr(ord('0') + oc.opportunity_no)
                last_opportunity_no = oc.opportunity_no

            oc.position = len(self.control_string)
            self.control_string += chr(ord('0') + oc.getOptimisticValue())
            self.position_map[oc.position] = oc

        dummy_oc = OptimisticChoice(1, -1, -1, '[n/a]','[n/a]', 'dummy','dummy',
                                    len(self.control_string))
        self.optimistic_choices.append(dummy_oc)
        self.position_map[len(self.control_string)] = dummy_oc

        logger.info(self.control_string)

    @staticmethod
    def getNumOptimisticChoices(control_string):
        return (len(control_string) - 3*control_string.count('c') -
                3*control_string.count('f'))

    def replacePosition(self, position, value):
        self.control_string = (self.control_string[:position] +
                               chr(ord('0') + value) +
                               self.control_string[position + 1:])

    def isFinished(self):
        return self.finished

    def computeStats(self, last_pos, current_pos):
        max_options = self.optimistic_choices[last_pos].max_options
        for i in range(last_pos, current_pos):
            max_options = max(max_options, self.optimistic_choices[i].max_options)
        stats = [0 for x in range(max_options)]
        i = last_pos
        while i < current_pos:
            value = (ord(self.control_string[self.getControlStringPositionForPosition(i)]) - ord('0'))
            while len(stats) <= value:
                stats.append(0)
            stats[value] += 1
            i += 1
        return stats

    def getOptimisticChoiceForPosition(self, pos):
        return self.optimistic_choices[pos]

    def getControlStringPositionForPosition(self, pos):
        return self.optimistic_choices[pos].position

    def advance(self, tries, n):
        last_oc = self.current_oc
        oc = self.getOptimisticChoiceForPosition(self.first_pos)
        category, kind = oc.category, oc.kind
        # advanced_oc = self.getOptimisticChoiceForPosition(self.first_pos + n)
        # advanced_category, advanced_kind = advanced_oc.category, advanced_oc.kind
        for i in range(n):
            oc.fix(oc.getOptimisticValue())

            self.first_pos += 1
            # if self.first_pos not in self.position_map:
                # continue
            oc = self.getOptimisticChoiceForPosition(self.first_pos)
            # oc = self.position_map[self.first_pos]
            self.current_oc = oc
            if oc.kind == kind and oc.category == category:
                continue

            stats = self.computeStats(self.oc_start_position, self.first_pos)
            logger.info(f' Finished   [{category}][{kind}] @ {self.first_pos:4} '
                        f' [try #{tries:4}]{stats!s}')
            self.oc_start_position = self.first_pos
            category, kind = oc.category, oc.kind

        if self.first_pos >= self.last_pos:
            # stats = self.computeStats(self.oc_start_position, self.last_pos)
            # logger.info(f' Finished   [{category}][{kind}] @ {self.first_pos:4} '
                        # f' [try #{tries:4}]{stats!s}')
            return

        if last_oc.kind == kind and last_oc.category == category:
            return

        logger.info(f' Working on [{oc.category}][{oc.kind}] '
                    f'@ {self.first_pos:4}')

    def generator(self):
        current_first_pos = self.first_pos
        initial_distance = self.last_pos - current_first_pos
        current_distance = initial_distance

        problems = []
        changed_since_problem = True

        logger.info(f' Working on [{self.current_oc.category}]'
                    f'[{self.current_oc.kind}] @ {self.first_pos:4} '
                    f'- {self.last_pos:4}')

        tries = 0
        time_cur = time.time()
        while ((self.max_tries is None or tries < self.max_tries) and
               (self.time_end is None or time_cur < self.time_end)):
            # current_distance = min(current_distance, self.last_pos - current_first_pos)

            if REPORT_EVERY_NUM_TRIES and tries % REPORT_EVERY_NUM_TRIES == 0:
                remaining_cs = self.control_string[self.getControlStringPositionForPosition(self.first_pos):]
                remaining = ChoiceExplorer.getNumOptimisticChoices(remaining_cs)
                rem_perc = (self.last_pos - self.first_pos) / (initial_distance)
                done_perc = int((1 - rem_perc) * 100)
                logger.info(f'Try: {tries}, Done: {done_perc:-3}%,'
                            f' Remaining: {remaining:-5}')

            if self.first_pos >= self.last_pos:
                return tries, False

            logger.debug(f' A problems: {problems}')
            logger.debug(f' cf: {current_first_pos}  cd: {current_distance}')
            last_problem_end = (0 if not problems else
                                problems[-1][0] + problems[-1][1])

            tries += 1
            current_last_pos = current_first_pos + current_distance
            while last_problem_end and current_first_pos >= last_problem_end:
                problems.pop()
                last_problem_end = (0 if not problems else
                                    problems[-1][0] + problems[-1][1])

            result = (yield self.control_string[:self.getControlStringPositionForPosition(current_last_pos)+1])
            assert(isinstance(result, bool))
            time_cur = time.time()

            if result is True:
                self.advance(tries, current_distance)
                current_first_pos = self.first_pos

                while (last_problem_end and
                       current_first_pos >= last_problem_end):
                    problems.pop()
                    last_problem_end = (0 if not problems else
                                        problems[-1][0] + problems[-1][1])

                if not problems:
                    assert self.first_pos >= self.last_pos
                    continue

                last_problem_end = problems[-1][0] + problems[-1][1]
                assert last_problem_end > current_last_pos

                current_distance = last_problem_end - current_last_pos
                if not changed_since_problem:
                    current_distance = max(1, int(current_distance / 2))

                assert current_distance > 0

                continue
            elif result is False:
                if current_distance > 1:
                    changed_since_problem = False
                    problems.append((current_first_pos, current_distance))
                    current_distance = max(1, int(current_distance / 2))
                    continue
            else:
                assert '  Expected a boolean value to be send'

            self.findAndLimitNextOptimisticChoice(tries, current_first_pos)
            current_first_pos = max(current_first_pos, self.first_pos)
            changed_since_problem = True

        if not (self.max_tries is None or tries < self.max_tries):
            logger.info(f'  Reached the limit of {self.max_tries} tries, '
                        f'limiting control string to {self.first_pos} positions '
                        f'out of {self.last_pos}')
        if not (self.time_end is None or time_cur < self.time_end):
            logger.info(f'  Reached the limit of {self.max_time} seconds, '
                        f'limiting control string to {self.first_pos} positions '
                        f'out of {self.last_pos}')

        for i in range(len(self.optimistic_choices)):
            oc = self.optimistic_choices[i]
            if i < self.first_pos:
                assert oc.fixed
            else:
                assert not oc.fixed
                oc.fix(0)

        if self.first_pos == 0:
            self.control_string = ''
        else:
            self.control_string = self.control_string[:self.getControlStringPositionForPosition(self.first_pos - 1)+1]
        return tries, True

    def findAndLimitNextOptimisticChoice(self, tries, current_first_pos):
        logger.debug(f'Old control string: {self.control_string[:self.getControlStringPositionForPosition(current_first_pos)+1]}')

        oc = self.getOptimisticChoiceForPosition(current_first_pos)
        assert(oc.getOptimisticValue() > 0)
        old_value = self.control_string[oc.position]
        new_value = ord(old_value) - 1 - ord('0')
        assert oc.getOptimisticValue() == new_value + 1
        oc.setOptimisticValue(new_value)
        self.replacePosition(oc.position, new_value)

        logger.debug(f'  Changed position {oc.position} from {old_value}'
                     f' to {new_value}: [{oc.category}][{oc.kind}]'
                     f'[{oc.name}][{oc.function}]')

        if new_value is 0:
            self.advance(tries, 1)


class Sourcefile(serializable):
    __type__ = 'SourceFile'

    def __init__(self, path, options=[], only_functions=[], output_file=''):
        assert(path.endswith('.c') or path.endswith('.cc') or
               path.endswith('.cpp'))
        self.path = path
        self.options = options
        self.only_functions = only_functions
        self.output_file = (output_file if output_file
                            else self.getFileName() + '.o')
        self.optimistic_choices = []
        self.current_control_string = ''

    def __repr__(self):
        return self.getFileName()

    def getFileName(self):
        return os.path.splitext(os.path.basename(self.path))[0]

    def getCompiler(self):
        if self.path.endswith('.c'):
            return 'clang'
        if self.path.endswith('.cc') or self.path.endswith('.cpp'):
            return 'clang++'
        assert(0 and "Don't know what compiler should be used!")

    def writeControl(self, optimistic_choices):
        if not ANNOTATE_SOURCE:
            return

        f2_c2cs = {}
        idx = 0
        for oc in optimistic_choices:
            if oc.opportunity_no < 0:
                continue
            assert oc.fixed
            f = oc.function.replace('.', '_')
            if f not in f2_c2cs:
                f2_c2cs[f] = {}
            c2cs = f2_c2cs[f]
            if oc.opportunity_no not in c2cs:
                c2cs[oc.opportunity_no] = ''
            c2cs[oc.opportunity_no] += chr(ord('0') + oc.getOptimisticValue())
            idx += 1

        ls = os.linesep

        lines = []
        with open(self.path, 'r') as fd:
            for line in fd.readlines():
                include = True
                for f, c2cs in f2_c2cs.items():
                    if f'KeepAlive_{f}' in line:
                        include = False
                        break
                    if f'{f}_OptimisticChoices = "' in line:
                        assert line.count('"') == 2
                        left = line.index('"') + 1
                        right = left + line[left:].index('"')
                        c2cs[-1] = line[left:right]
                        include = False
                        break
                if include:
                    lines.append(line)

        lines.append('')
        lines.append('')
        for f, c2cs in f2_c2cs.items():
            pre_cs = ''
            cs = ''
            for k, v in c2cs.items():
                if k >= 0:
                    cs += '#c' + chr(k + ord('0')) + v
                else:
                    assert not pre_cs
                    pre_cs = v
            cs = pre_cs + cs

            lines.append(f'static const char *{f}_OptimisticChoices = "{cs}";')
            lines.append(f'{ls}const char **KeepAlive_{f}_'
                         f'{self.getFileName()} = ')
            lines.append(f'&{f}_OptimisticChoices;{ls}')

        with open(self.path, 'w') as fd:
            fd.writelines(lines)


class InputOutputPair(serializable):
    __type__ = 'InputOutputPair'

    def __init__(self, input, output, timeout, returncode=0, use_stdout=True,
                 use_stderr=True):
        self.input = input
        self.output = output
        self.timeout = timeout
        self.returncode = returncode
        self.use_stdout = use_stdout
        self.use_stderr = use_stderr


class Benchmark(serializable):
    __type__ = 'Benchmark'

    def __init__(self, name, source_files, options, executable,
                 input_output_pairs, verify_cmd='', verify_cmd_timeout=86400,
                 make_cmd='make'):
        assert(isinstance(options, collections.Iterable) and
               all([isinstance(x, str) for x in options]))

        self.name = name
        self.source_files = []
        self.input_output_pairs = []
        self.options = options
        self.executable = executable
        self.make_cmd = make_cmd
        self.verify_cmd = verify_cmd
        self.verify_cmd_timeout = verify_cmd_timeout
        self._num_out_versions = 0

        for source_file in source_files:
            if isinstance(source_file, Sourcefile):
                self.source_files.append(source_file)
            elif isinstance(source_file, str):
                self.source_files.append(Sourcefile(source_file))
            else:
                raise ValueError('Source files of a benchmark need to be '
                                 '"Sourcefile" objects or strings describing '
                                 'the relative path of the file')

        for io_pair in input_output_pairs:
            if isinstance(io_pair, InputOutputPair):
                self.input_output_pairs.append(io_pair)
            elif (isinstance(io_pair, tuple) and len(io_pair) == 2 and
                  isinstance(io_pair[0], str) and isinstance(io_pair[1], str)):
                self.input_output_pairs.append(InputOutputPair(*io_pair))
            else:
                raise ValueError('Input/Output pairs benchmark need to be '
                                 '"InputOutputPair" objects or a string pairs')


class Experiment(serializable):
    __type__ = 'Experiment'

    def __init__(self, benchmark_files, max_tries=None, max_time=None,
                 oc_blacklist=[], oc_whitelist=None):
        self.benchmark_files = benchmark_files
        self.max_tries = max_tries
        self.max_time = max_time
        self.time_end = (time.time() + max_time) if max_time else None
        self.oc_blacklist = (oc_blacklist if isinstance(oc_blacklist, list)
                             else [oc_blacklist])
        self.oc_whitelist = oc_whitelist if oc_whitelist != [] else None
        self.annotation_run = 0

    def run(self):
        base_path = os.path.abspath(os.curdir)

        for benchmark_file in self.benchmark_files:
            # Reset the base direcotry
            os.chdir(base_path)

            benchmark_file = os.path.abspath(benchmark_file)
            benchmark = self.readBenchmarkFile(benchmark_file)
            if not benchmark:
                continue

            assert isinstance(benchmark, Benchmark)

            try:
                self.runBenchmark(benchmark, benchmark_file)
            except Exception as e:
                logger.error(f' The execution of {benchmark} ended in an '
                             f' uncaught exception:\n{e!s}', exc_info=True)

    def readBenchmarkFile(self, benchmark_file):
        if not os.path.isfile(benchmark_file):
            logger.error(f'Benchmark file @ {benchmark_file} does not exist')
            return None

        try:
            with open(benchmark_file, 'r') as fd:
                return Benchmark.from_json(fd.read())
        except Exception as e:
            logger.error(f'Failed to read benchmark file @ {benchmark_file}:\n'
                         f'{e}')
            return None

    def runBenchmark(self, benchmark, benchmark_file):
        logger.info(f'Start benchmark {benchmark.name}')
        benchmark_path = os.path.dirname(benchmark_file)

        success = False
        logger.debug(f'- Change base directory to: {benchmark_path}')
        try:
            os.chdir(benchmark_path)
        except Exception as e:
            logger.error(f'Failed to change path to "{benchmark_path}":\n{e}')
        else:
            if self.makeAndVerify(benchmark, initial=True):
                logger.info(f'- Initial build successful, proceed to '
                            f'optimistic optimization for '
                            f'{len(benchmark.source_files)} source files')
                for source_file in benchmark.source_files:
                    logger.info(f'- Source file is {source_file.path}')
                    benchmark._num_out_versions = 0
                    success = self.optimizeAndRun(benchmark, source_file)
                    if success and source_file.current_control_string:
                        logger.info(f'- Optimistic optimization of '
                                    f'{source_file} from {benchmark.name} '
                                    f'successful.{os.linesep}- Generated '
                                    f'{benchmark._num_out_versions} different '
                                    f'output versions{os.linesep}- Final '
                                    f'control string: '
                                    f'{source_file.current_control_string}')
                        source_file.writeControl(source_file.optimistic_choices)
                    else:
                        logger.info(f'- Optimistic optimization of '
                                    f'{source_file} from {benchmark.name} '
                                    f'unsuccessful.')
            else:
                logger.info(f'- Initial build of {benchmark.name} failed')

        logger.info(f'Finished benchmark {benchmark.name}, '
                    f'{"" if success else "un"}successful')

    def optimizeAndRun(self, benchmark, source_file):
        logger.debug(f' Determine optimistic optimization choices for '
                     f'{source_file} in {benchmark.name}')

        stop = False
        self.annotation_run = 0
        while self.annotation_run < 14 and not stop:
            self.annotation_run += 1
            logger.debug(f' Determine optimistic optimization coices for '
                         f'annoator run number {self.annotation_run}')
            try:
                num_op, ocs = self.determineOptimisticChoices(benchmark,
                                                              source_file)
                assert isinstance(num_op, int)
                assert isinstance(ocs, collections.Iterable)
                assert all([isinstance(oc, OptimisticChoice) for oc in ocs])
            except Exception as e:
                logger.error(f'Unexpected error:\n{e!s}', exc_info=True)
                return False

            if not ocs:
                continue;

            ocs.sort(key=lambda oc: (oc.opportunity_no, oc.function_no))
            choice_explorer = ChoiceExplorer(self.max_tries, self.max_time,
                                             self.time_end, num_op, ocs,
                                             source_file.current_control_string)
            try:
                it = choice_explorer.generator()
                control_string = next(it)
                while True:
                    logger.debug(f'  Control string: '
                                f'~{ChoiceExplorer.getNumOptimisticChoices(control_string)}\
                                ')
                    success = self.compileSourceOptimistically(benchmark,
                                                               source_file,
                                                               control_string)
                    control_string = it.send(success)
            except StopIteration as e:
                tries, stop = int(e.value[0]), bool(e.value[1])
                control_string = choice_explorer.control_string
                logger.info(f' Final control string with '
                            f'~{ChoiceExplorer.getNumOptimisticChoices(control_string)} '
                            f'optimistic choices generated after {tries} tries')
                success = self.compileSourceOptimistically(benchmark,
                                                           source_file,
                                                           control_string,
                                                           force_validation=True)
                source_file.optimistic_choices += choice_explorer.optimistic_choices[:choice_explorer.first_pos]
                source_file.current_control_string = control_string
                if not success:
                    print(control_string)
                assert success
                continue
            except Exception as e:
                logger.error(f'Unexpected error:\n{e!s}', exc_info=True)
                return False

        return True

    def compileSourceOptimistically(self, benchmark, source_file,
                                    control_string, compile_only=False,
                                    force_validation=False):

        # executable_path = os.path.abspath(benchmark.executable)
        executable_path = benchmark.executable
        if os.path.isfile(executable_path):
            logger.debug(f'  Delete existing executable @ {executable_path}')
            os.remove(executable_path)

        compiler = source_file.getCompiler()
        options = source_file.options + benchmark.options
        try:
            cmd = [compiler, *options, '-mllvm',
                   '-optimistic-annotations-control=' + control_string,
                   source_file.path]
            if source_file.only_functions:
                cmd += ['-mllvm',
                        f'-optimistic-annotator-only-functions='
                        f'{",".join(source_file.only_functions)}']
            # print(' '.join(cmd))
            run_result = sp.run(cmd, stdout=sp.DEVNULL, stderr=sp.DEVNULL)
            if run_result.returncode is not 0:
                logger.warn(f'   - Compile error, exit code was '
                            f'{run_result.returncode}:\n'
                            f'     - Command: {" ".join(cmd)}')
                return False
        except Exception as e:
            logger.warn(f'   - Compile error:\n'
                        f'     - Command: {" ".join(cmd)}\n'
                        f'     - {e!s}')
            return False

        if compile_only:
            return True

        logger.debug(f' Optimistic compilation with '
                     f'~{ChoiceExplorer.getNumOptimisticChoices(control_string)} '
                     f'optimistic choices done.')

        logger.debug(f' Compare output to last valid output version [{control_string}]')
        if filecmp.cmp(os.path.join(temp_directory,
                                    source_file.output_file +
                                    f'.{benchmark._num_out_versions}'),
                       source_file.output_file, shallow=False):
            logger.debug(f' Files match, no change to the output')
            if force_validation:
                logger.debug(f' Validation forced!')
            else:
                return True
        else:
            logger.debug(f' Files do not match, continue with verification')

        if not self.makeAndVerify(benchmark, control_string=control_string):
            output_path = os.path.join(temp_directory,
                                       source_file.output_file + ".broken")
            if os.path.isfile(output_path):
                os.remove(output_path)
            if os.path.isfile(source_file.output_file):
                shutil.copyfile(source_file.output_file, output_path)

            output_path = os.path.join(temp_directory,
                                       executable_path +'.broken')
            if os.path.isfile(output_path):
                os.remove(output_path)
            if os.path.isfile(executable_path):
                shutil.copyfile(executable_path, output_path)
            return False

        benchmark._num_out_versions += 1
        logger.debug(f' Files did not match and verification was successful, '
                     f'store output as last valid'
                     f' version (no {benchmark._num_out_versions})')

        output_path = os.path.join(temp_directory,
                                   source_file.output_file +
                                   f'.{benchmark._num_out_versions}')
        shutil.copyfile(source_file.output_file, output_path)

        output_path = os.path.join(temp_directory,
                                   executable_path +
                                   f'.{benchmark._num_out_versions}')
        shutil.copyfile(executable_path, output_path)
        # try:
            # sp.run(f'objdump -d "{output_path}" "{output_path}.s"'.split(' '),
                   # stdout=sp.DEVNULL, stderr=sp.DEVNULL)
        # except Exception:
            # pass

        return True

    def determineOptimisticChoices(self, benchmark, source_file):
        compiler = source_file.getCompiler()
        options = source_file.options + benchmark.options
        try:
            cmd = [compiler, *options, '-mllvm',
                   f'-optimistic-annotation-runs={self.annotation_run}',
                   '-mllvm', '-print-optimistic-opportunities',
                   '-mllvm', '-optimistic-annotations-control=' +
                   f'{source_file.current_control_string}',
                   source_file.path]
            if source_file.only_functions:
                cmd += ['-mllvm', f'-optimistic-annotator-only-functions={",".join(source_file.only_functions)}']

            run_result = sp.run(cmd, stdout=sp.DEVNULL, stderr=sp.PIPE)
            if run_result.returncode is not 0:
                logger.warn(f'   - Compile error, exit code was '
                            f'{run_result.returncode}:\n'
                            f'     - Command: {" ".join(cmd)}')
                return (0, [])

        except Exception as e:
            logger.error(f'Unexpected error:\n{e}', exc_info=True)
            return (0, [])

        oc_re = re.compile(r'^\[OC\]'
                           r'\[(\d+)\]'
                           r'\[\d+\]'
                           r'\[(\d+)\]'
                           r'\[(\d+)\]'
                           r'\[([^\]]+)\]'
                           r'\[([^\]]+)\]'
                           r'\s*@ ([^\s]*) in (.*)',
                           re.MULTILINE)

        run_output = run_result.stderr.decode('utf8')
        matches = oc_re.findall(run_output)
        if not matches:
            logger.info(f' No opportunities for optimistic optimization found'
                        f' ({" ".join(cmd)})')
            return (0, [])

        num_opportunities = len(matches)
        logger.debug(f'  - Found {num_opportunities} optimistic optimization '
                     f'opportunities')

        optimistic_choices = []
        position = 0
        for match in matches:
            assert isinstance(match, collections.Iterable)
            assert len(match) is 7
            oc = OptimisticChoice(*match, position)
            if self.oc_whitelist is not None and (f'[{oc.category}][{oc.kind}]'
                                                  not in self.oc_whitelist):
                oc.fix(0)
                continue
            if f'[{oc.category}][{oc.kind}]' in self.oc_blacklist:
                oc.fix(0)
                continue
            position += 1
            optimistic_choices.append(oc)

        logger.debug(f' Filtered {num_opportunities-len(optimistic_choices)} '
                     f'optimistic optimization opportunities based on '
                     f'experiment filter')
        if not optimistic_choices:
            logger.info(f' No opportunities for optimistic optimization left')
            return (0, [])

        logger.info(f' Optimistic choices to work with: '
                    f'{len(optimistic_choices)}:')

        oc = optimistic_choices[0]
        category, kind, position, num = oc.category, oc.kind, oc.position, 0
        for oc in optimistic_choices:
            if oc.category == category and oc.kind == kind:
                num += 1
                continue
            logger.info(f'  - [{category}][{kind}] : {num:6} times starting '
                        f'@ {position:10}')
            category, kind, position, num = oc.category, oc.kind, oc.position, 1
        logger.info(f'  - [{category}][{kind}] : {num:6} times starting '
                    f'@ {position:10}')

        return (num_opportunities, optimistic_choices)

    def makeAndVerify(self, benchmark, initial=False, control_string=''):
        # executable_path = os.path.abspath(benchmark.executable)
        executable_path = benchmark.executable

        if initial:
            for source_file in benchmark.source_files:
                logger.debug(f'   - Compile {source_file.path} without '
                             f'optimistic choices')
                # Path(source_file.path).touch()
                success = self.compileSourceOptimistically(benchmark,
                                                           source_file,
                                                           control_string='',
                                                           compile_only=True)
                if not success:
                    logger.warn(f'   - Non-optimistic compilation of '
                                f'{source_file.path} failed')
                    return False


        if not os.path.isfile(executable_path) or initial:
            if os.path.isfile(executable_path):
                logger.debug(f'   - Delete existing executable @ '
                             f'{executable_path}')
                os.remove(executable_path)

            logger.debug(f'   - Build benchmark ({benchmark.make_cmd})')
            try:
                sp.run(benchmark.make_cmd.split(' '), stdout=sp.DEVNULL,
                       stderr=sp.DEVNULL)
            except Exception as e:
                logger.warn(f'   - Build ({benchmark.make_cmd}) failed:\n{e}')
                return False

        logger.debug(f'   - Check for executable @ {benchmark.executable}')
        if not os.path.isfile(executable_path):
            logger.debug(f'    - No executable found @ {executable_path}')
            return False

        if initial:
            for source_file in benchmark.source_files:
                logger.debug(f'   - Check for compiled source file @ '
                             f'{source_file.output_file}')
                if not os.path.isfile(source_file.output_file):
                    logger.error(f'    - No output file found')
                    return False

                logger.debug(f' Copy object file to {temp_directory} '
                             f'(last valid version no '
                             f'{benchmark._num_out_versions})')
                output_path = os.path.join(temp_directory,
                                           source_file.output_file +
                                           f'.{benchmark._num_out_versions}')
                shutil.copyfile(source_file.output_file, output_path)
                assert os.path.isfile(output_path)

                output_path = os.path.join(temp_directory,
                                           executable_path +
                                           f'.{benchmark._num_out_versions}')
                shutil.copyfile(executable_path, output_path)
                assert os.path.isfile(output_path)

        if benchmark.verify_cmd:
            logger.debug(f'   - Run verify command {benchmark.verify_cmd}')
            try:
                run_result = sp.run(benchmark.verify_cmd.split(' '),
                                    stdout=sp.DEVNULL, stderr=sp.DEVNULL,
                                    timeout=benchmark.verify_cmd_timeout)
                if run_result.returncode == 0:
                    logger.debug(f'   - Verify command determined match')
                else:
                    logger.debug(f'   - Verify command determined mismatch')
                    return False
            except Exception as e:
                logger.warn(f'   - Verify command failed:\n{e}')
                return False

        if benchmark.input_output_pairs:
            logger.debug(f'   - Start verification of '
                         f'{len(benchmark.input_output_pairs)} input/output'
                         f' pairs')
            for io_pair in benchmark.input_output_pairs:
                assert isinstance(io_pair, InputOutputPair)
                try:
                    cmd = [executable_path, *io_pair.input]
                    if not self.runAndVerify(cmd, io_pair, initial,
                                             control_string):
                        return False
                except Exception as e:
                    logger.warn(f'Uncaught exception during run and verify:\n'
                                f'{e}')
                    return False

        logger.debug(f'    - Verification successful')
        return True

    def runAndVerify(self, cmd, io_pair, initial, control_string):
        stdout_pipe = sp.PIPE if io_pair.use_stdout else sp.DEVNULL
        if not io_pair.use_stderr:
            stderr_pipe = sp.DEVNULL
        elif io_pair.use_stdout:
            stderr_pipe = sp.STDOUT
        else:
            stderr_pipe = sp.PIPE
        stdin = sp.DEVNULL

        logger.debug(f'    - Run command "{" ".join(cmd)}"')
        try:
            for arg in cmd:
                if arg.startswith('<') and os.path.isfile(arg[1:]):
                    stdin = open(arg[1:], 'r')
            run_result = sp.run(cmd, stdout=stdout_pipe, stderr=stderr_pipe,
                                timeout=io_pair.timeout, stdin=stdin)
            if stdin != sp.DEVNULL:
                stdin.close()
        except sp.TimeoutExpired:
            logger.debug(f'     - Run failed due to time out ({io_pair.timeout}s)')
            try:
                if stdin != sp.DEVNULL:
                    stdin.close()
            except Exception:
                pass
            return False
        except Exception as e:
            logger.warn(f'     - Run failed due to unknown error:\n{e!s}')
            try:
                if stdin != sp.DEVNULL:
                    stdin.close()
            except Exception:
                pass
            return False

        logger.debug(f'    - Check return value')
        if run_result.returncode is not io_pair.returncode:
            logger.debug(f'  Run failed due to exit code mismatch, expected '
                         f'{io_pair.returncode} got {run_result.returncode}')
            return False

        logger.debug(f'    - Collect run output')
        run_output = ''
        if os.path.isfile('output.txt'):
            with open('output.txt', 'r') as fd:
                run_output = fd.read()
        else:
            if io_pair.use_stdout:
                run_output += run_result.stdout.decode('utf8')
            elif io_pair.use_stderr:
                run_output += run_result.stderr.decode('utf8')

        logger.debug(f'    - Try to match output with expected pattern')
        expected_output = io_pair.output
        if os.path.isfile(io_pair.output):
            with open(io_pair.output, 'r') as fd:
                expected_output = fd.read()
        # if expected_output.endswith(os.linesep):
            # expected_output = expected_output[:-len(os.linesep)]

        match = re.fullmatch(expected_output, run_output)
        if match:
            logger.debug(f'    - Output matched expected pattern '
                         f'{len(expected_output)} vs {len(run_output)}')
            return True

        logger.debug(f'    - Output did not match expected pattern')
        if STORE_MISMATCH_PATH:
            with open(STORE_MISMATCH_PATH, 'a') as fd:
                fd.write(f'Command: {" ".join(cmd)}{os.linesep}')
                fd.write(f'Control: {control_string}{os.linesep}')
                fd.write(f'I/O pair: {io_pair.input} / '
                         f'{io_pair.output}{os.linesep}')
                fd.write(run_output + os.linesep * 3)

        if initial:
            run_output = run_output.splitlines()
            expected_output = expected_output.splitlines()
            for i, expected_line in enumerate(expected_output):
                if i >= len(run_output):
                    break
                result = re.fullmatch(expected_line, run_output[i])
                if result is None:
                    logger.debug(f'    - Run failed due to output mismatch in'
                                 f' line {i}:\n"{expected_line!r}"\n'
                                 f'"{run_output[i]!r}"')
        return False


for cls in [Sourcefile, InputOutputPair, Benchmark, Experiment]:
    serializable.classes[cls.__type__] = cls

benchmark_files = ['./test/benchmark.ot']

if len(sys.argv) > 1:
    benchmark_files = sys.argv[1:]

oc_blacklist= []#['[Par][Alignment]', '[Mem][Alignment]', '[Mem][ResAlign ]']
oc_whitelist = []#['[Fn][RetNoAlia ]','[Par][NoAlias  ]']

#max_time is in seconds!
ex = Experiment(benchmark_files, #max_time=5,
                oc_whitelist=oc_whitelist,
                oc_blacklist=oc_blacklist)
ex.run()

# Dump an experiment (or anything serializable) to json:
#   ex.to_json()

# Read an experiment (or anything serializable) from json in file:
#   with open(file, 'r') as fd:
#       return Experiment.from_json(fd.read())
