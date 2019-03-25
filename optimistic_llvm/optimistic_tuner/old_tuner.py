
sourcefiles = ['CalculateXS.c']
sourcefiles = ['./force_types/force_lj_neigh.cpp']
sourcefiles = ['distribution.c']

command = ('clang -std=gnu99 -Wall -fopenmp -O3 '
           '-march=native -DVERIFICATION -c -o %s.o %s ')

command = ('clang++'
           ' -I./ -I../../kokkos/core/src -I../../kokkos/containers/src'
           ' -I../../kokkos/algorithms/src -I../../kokkos/core/src/eti'
           ' --std=c++11 -fopenmp=libomp --gcc-toolchain=/usr -O3 -g'
           ' -fno-omit-frame-pointer -o %s.o'
           ' -I/home/johannes/repos/argonne/ExaMiniMD/src'
           ' -I/home/johannes/repos/argonne/ExaMiniMD/src/force_types'
           ' -I/home/johannes/repos/argonne/ExaMiniMD/src/comm_types'
           ' -I/home/johannes/repos/argonne/ExaMiniMD/src/neighbor_types'
           ' -I/home/johannes/repos/argonne/ExaMiniMD/src/binning_types'
           '  -c /home/johannes/repos/argonne/ExaMiniMD/src/%s')

command = ('clang -g -O3 -Wall -Wno-deprecated -std=gnu99 -DDFFT_TIMING=2'
           ' -I/usr/include -c -o build/%s.o %s')

# source_base = os.path.splitext(os.path.basename(sourcefiles[0]))[0]
# print(command % (source_base, sourcefiles[0]))

versions = [('./XSBench -p 100 -g 100 -l 100'.split(' '),
             'checksum', 'Verification checksum: 915442'
             ' (WARNING - INAVALID CHECKSUM!)', 1),
            ('./XSBench -p 1000 -g 1000 -l 1000'.split(' '),
             'checksum', 'Verification checksum: 773837'
             ' (WARNING - INAVALID CHECKSUM!)', 1.8)]
            # ('./XSBench -s small'.split(' '),
             # 'checksum', 'Verification checksum: 211645 (Valid)', 15)]

versions = [('./ExaMiniMD -il ../input/in_small.lj'.split(' '),
             '',
             '10 1.266844 -6.133420 -4.233161',
             1.5)]

versions = [('./build/TestDfft 1 50 50 50'.split(' '),
             '',
             'a[0,0,0] = (125000.000000,0.000000) = (40fe848000000000,0)',
             1),
            ('./build/TestDfft 1 51 51 51'.split(' '),
             '',
['a[0,0,0] = (132651.000000,0.000000) = (4100315800000000,0)',
'real in [-1.347061e-12,2.146878e-12] = [bd77b29f0f679daa,3d82e256c04691a3]',
'imag in [-1.589406e-12,1.589406e-12] = [bd7bf60c5e314383,3d7bf60c5e314384]'],
             1)]

OptimizationTypes = set()
# OptimizationTypes.add('[CFG][BranchTrg]')
OptimizationTypes.add('[Par][NoCapture]')
OptimizationTypes.add('[Par][NoAlias  ]')
OptimizationTypes.add('[Par][Dereferen]')
OptimizationTypes.add('[Par][Alignment]')
# OptimizationTypes.add('[Par][MemAccess]')
OptimizationTypes.add('[Fn][NoUnwind  ]')
OptimizationTypes.add('[Fn][MemAccess ]')
OptimizationTypes.add('[Fn][ArgMemOnly]')
OptimizationTypes.add('[Fn][ArgMemInac]')
OptimizationTypes.add('all')
# print(OptimizationTypes)


def check(check_cmd, key, result, timeout):
    try:
        run_result = sp.run(check_cmd, stdout=sp.PIPE, stderr=sp.PIPE,
                            timeout=timeout)
    except:
        print(f'Timeout of {timeout}s reached!')
        return False

    if run_result.returncode is not 0:
        print(f'Returncode: {run_result.returncode}')
        return False

    Found = False
    lines = run_result.stdout.decode('utf8').split('\n')
    for line in lines:
        if key:
            if key not in line:
                continue
            if not line.startswith(result):
                print(f'Mismatch: "{result}" vs "{line}"')
                return False
            Found = True
        elif type(result) is not str:
            if line in result:
                Found = True
        else:
            if line.startswith(result):
                Found = True

    if not Found:
        print(f'Could not find "{result}" in output')

    return Found


# os.system('make clean')
run_result = sp.run(['make'])
if run_result.returncode != 0:
    print('Make failed!')

for source in sourcefiles:
    print(f'Initial compile of {source}')
    source_base = os.path.splitext(os.path.basename(source))[0]
    run_result = sp.run((command % (source_base, source)).split(' '),
                        stdout=sp.PIPE, stderr=sp.PIPE)
    assert(run_result.returncode == 0)

run_result = sp.run(['make'])
if run_result.returncode != 0:
    print('Make failed!')
assert(all(check(*version) for version in versions))
print('Initial verification done!')

def try_and_check(cmd, fixed, intervals):
    if len(intervals) == 0:
        return fixed

    num_trivial_fixed = 0
    first_interval = intervals[0]
    intervals = intervals[1:]
    first_interval_length = len(first_interval)
    while (num_trivial_fixed < first_interval_length and
           first_interval[num_trivial_fixed] == '0'):
        fixed += '0'
        num_trivial_fixed += 1

    if num_trivial_fixed > 0:
        first_interval = first_interval[num_trivial_fixed:]
        first_interval_length = first_interval_length - num_trivial_fixed
        print(f'Trivially fixed the first {num_trivial_fixed} values to "0"')

    if first_interval_length == 0:
        return try_and_check(cmd, fixed, intervals)

    first_remaining_zero_pos = first_interval.find('0')
    assert(first_remaining_zero_pos == -1 or
           first_interval[first_remaining_zero_pos] == '0')
    if first_remaining_zero_pos > 0:
        if len(intervals) == 0:
            intervals = [first_interval[first_remaining_zero_pos:]] + intervals
        else:
            intervals = [first_interval[first_remaining_zero_pos:] + intervals[0]] + intervals[1:]
        first_interval = first_interval[:first_remaining_zero_pos]
        first_interval_length = first_remaining_zero_pos

    print('try interval of length ', first_interval_length, ' with',
          (first_interval_length - ''.join(first_interval).count('0')),
          'optimistic choices out of ', len(intervals), 'intervals (remaining',
          len(''.join(intervals)), ').')

    print(cmd + fixed + first_interval)
    run_result = sp.run((cmd + fixed + first_interval).split(' '), stderr=sp.PIPE)
    if run_result.returncode != 0:
        print('ERROR:\n%s' % (run_result.stderr.decode('utf8')))
    else:
        run_result = sp.run(['make'])
        if run_result.returncode != 0:
            print('Make failed!')

        if all(check(*version) for version in versions):
            print(f' success! Add {first_interval_length} optimistic choices')
            return try_and_check(cmd, fixed + first_interval, intervals)

    if first_interval_length == 1:
        if first_interval[0] == '1':
            print(f' failure! Add non optimistic choice!')
            return try_and_check(cmd, fixed + '0', intervals)
        print(f' failure! Restrict optimistic choice!')
        return try_and_check(cmd, fixed, [chr(ord(first_interval[0])-1)] + intervals)

    if len(intervals) == 0 or first_interval_length < 16:
        new_intervals = [first_interval[:int(first_interval_length/2)],
                        first_interval[int(first_interval_length/2):]] + intervals
        assert(len(new_intervals[0]) + len(new_intervals[1]) == first_interval_length)
    else:
        new_intervals = [first_interval[:int(first_interval_length/2)],
                        (first_interval[int(first_interval_length/2):]
                         + intervals[0])] + intervals[1:]
    return try_and_check(cmd, fixed, new_intervals)


def getBrackets(s, first, last):
    assert(first <= last)
    assert(s.count('[') > last)
    assert(s.count(']') > last)
    beg = s.find('[')
    while first > 0:
        old = beg
        beg = s.find('[', beg + 1)
        assert(s[old:beg].count(']') == 1)
        first -= 1
        last -= 1
    end = s.find(']', beg + 1)
    while last > 0:
        end = s.find(']', end + 1)
        last -= 1
    return s[beg:end+1]


for source in sourcefiles:
    print(source)
    SkippedTypes = set()
    source_base = os.path.splitext(os.path.basename(source))[0]

    run_result = sp.run((command % (source_base, source) +
                         ' -mllvm -print-optimistic-choices').split(' '),
                        stdout=sp.PIPE, stderr=sp.PIPE)
    lines = run_result.stderr.decode('utf8').split('\n')
    print('Got %i opportunities for optimistic choices!' % (len(lines)))

    choice = ""
    for line in lines:
        if not line.startswith('['):
            continue
        if not '[OC][' in line or ' ' not in line:
            continue
        opt_type = getBrackets(line, 3, 4)
        if opt_type in OptimizationTypes or 'all' in OptimizationTypes:
            MaxChoices = int(line[5:5+line[5:].index(']')])
        else:
            MaxChoices = 1
            if opt_type not in SkippedTypes:
                print(f"Skip opportunity of {type}")
                SkippedTypes.add(opt_type)
        choice += str(MaxChoices - 1)

    print('Initial optimistic choice:\n', choice)

    cmd = (command % (source_base, source)) + ' -mllvm -optimistic-annotations-control='
    choice = try_and_check(cmd, '', [choice])

    print('Final optimistic choice:\n', choice)
    sp.run((cmd + choice).split(' '), stdout=sp.PIPE, stderr=sp.PIPE)
    run_result = sp.run(['make'])
    if run_result.returncode != 0:
        print('Make failed!')
    assert(all(check(*version) for version in versions))

    with open(source, 'a') as fd:
        fd.write('%s%sstatic const char *OptimisticChoices%s = "%s";'
                 '%sconst char **KeepAlive_%s = &OptimisticChoices%s;%s' %
                 (os.linesep, os.linesep, source_base, choice, os.linesep,
                  source_base, source_base, os.linesep))
