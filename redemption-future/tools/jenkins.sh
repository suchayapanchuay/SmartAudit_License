#!/usr/bin/env bash

# cd /var/lib/jenkins/jobs/redemption-future/workspace
cd $(realpath -m "$0/../..")

if ! [[ $PWD =~ ^/var/lib/jenkins/jobs/* ]]; then
  echo 'Please, run script on Jenkins.'
  exit 1
fi

set -ex

typeset -i fast=0
typeset -i update=0
for act in "$@" ; do
    case "$act" in
        fast) fast=1 ;;
        update) update=1 ;;
        *)  set +x ; echo "$0 [fast] [update]

fast: enable fast compilation
update: update targets.jam and others generated files";
            exit 1 ;;
    esac
done

timestamp=$(date +%s)
show_duration()
{
    local timestamp2=$(date +%s)
    echo duration"[$@]": $((($timestamp2-$timestamp)/60))m $((($timestamp2-$timestamp)%60))s
    timestamp=$timestamp2
}


# lua analyzer
if (( $fast == 0 )); then
    ./tools/c++-analyzer/redemption-analyzer.sh
    show_duration redemption-analyzer.sh


    # Python tests
    # @{
    python_test()
    {
        pushd "$1"
        python3 -m unittest discover -t . tests
        popd
    }

    python_test tools/sesman
    python_test tools/conf_migration_tool

    show_duration "python tests"
    # @}


    # Lua tests
    # @{
    eval "$(luarocks path)"

    ./tools/cpp2ctypes/test.sh

    show_duration "lua tests"
    # @}
fi


rm_nofast() {
    if (( $fast == 0 )); then
        rm -r "$@"
    fi
}

# jsclient (emscripten)
if [[ "$BUILD_EMSCRIPTEN" != '0' ]]; then
    pushd projects/jsclient
    source ~/emsdk/emsdk_env.sh
    rm_nofast bin
    #version=$(clang++ --version | sed -E 's/^.*clang version ([0-9]+\.[0-9]+).*/\1/;q')
    echo "using clang : : clang++ ;" > project-config.jam
    if [[ ! -d system_include/boost ]]; then
        mkdir -p system_include
        ln -s /usr/include/boost/ system_include
    fi
    if [[ ! -d node_modules ]]; then
        ln -s ~/node_jsclient/future/node_modules .
    fi
    set -o pipefail
    toolset_emscripten=toolset=clang
    bjam -qj2 $toolset_emscripten debug cxxflags=-Wno-shadow-field |& sed '#^/var/lib/jenkins/jobs/redemption-future/workspace/##'
    set +o pipefail
    rm_nofast bin/*
    popd
fi

show_duration jsclient


#These following packages MUST be installed. See README of redemption project
#aptitude install build-essential bjam boost-build libboost-program-options-dev libboost-test-dev libssl-dev locales cmake

# # use libstdc++-8 with clang-8 because version 9 fails with -D_GLIBCXX_DEBUG (ok with clang-9)
# libstdcxx_compact_version=8
# mkdir -p \
#     libstdc++-compact/include/c++ \
#     libstdc++-compact/lib/gcc/x86_64-unknown-linux-gnu
# ln -s /usr/include/c++/$libstdcxx_compact_version libstdc++-compact/include/c++
# ln -s /usr/lib/gcc/x86_64-linux-gnu/$libstdcxx_compact_version libstdc++-compact/lib/gcc/x86_64-unknown-linux-gnu

# BJAM Build Test
echo -e "" > project-config.jam
valgrind_compiler=gcc-12
toolset_gcc=toolset=gcc
toolset_wab=toolset=gcc-12
gcovbin=gcov
toolset_clang=toolset=clang

export REDEMPTION_TEST_DO_NOT_SAVE_IMAGES=1
export LSAN_OPTIONS=exitcode=0 # re-trace by valgrind
export UBSAN_OPTIONS=print_stacktrace=1

export BOOST_TEST_COLOR_OUTPUT=0

if (( $fast == 0 )); then
    rm -rf bin
else
    rm -rf bin/tmp/
fi

mkdir -p bin/tmp
export TMPDIR_TEST=bin/tmp/

# export REDEMPTION_LOG_PRINT=1
export REDEMPTION_LOG_PRINT=0
export cxx_color=never

export BOOST_TEST_RANDOM=$RANDOM
echo random seed = $BOOST_TEST_RANDOM

build()
{
    bjam -q "$@" || {
        local e=$?
        export REDEMPTION_LOG_PRINT=1
        bjam -q "$@" -j1
        exit $e
    }
}

build_all() {
    build "$@" -j4 ocr_tools tests/utils tests/transport tests/acl \
        tests/system tests/gdi tests/keyboard
    # build "$@" -j1 exe libs tests/capture tests/lib tests/server \
    #     tests/client_mods tests/mod/rdp.norec tests/mod/vnc.norec
    build "$@" -j2
}

rootlist()
{
    ls -1
}

# implicitly created by bjam
mkdir -p bin
beforerun=$(rootlist)

# release for -Warray-bounds and not assert
build_all $toolset_wab cxxflags=-g

show_duration $toolset_wab

if (( $fast == 0 )); then
    # valgrind
    #find ./bin/$gcc/release/tests/ -type d -exec \
    #  ./tools/c++-analyzer/valgrind -qd '{}' \;
    /usr/bin/time --format="%Es - %MK" \
      find ./bin/$valgrind_compiler/release/*/tests/ -type d -exec \
      parallel -j2 ./tools/c++-analyzer/valgrind -qd ::: '{}' +

    show_duration valgrind
fi

rm_nofast bin/gcc*


# Warn new files created by tests.
set -o pipefail
diff <(echo "$beforerun") <(rootlist) | while read l ; do
    echo "Jenkins:${diffline:-0}:0: warnings: $l [directory integrity]"
    ((++diffline))
done || if (( $fast == 1 )); then
    # error with fast compilation
    echo "Directory integrity error: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
    exit 1
fi
set +o pipefail

build_all $toolset_clang -sNO_FFMPEG=1 san -s FAST_CHECK=1
rm_nofast bin/clang*

show_duration $toolset_clang


if (( $fast == 0 )); then
    # debug with coverage
    build_all $toolset_gcc debug -s FAST_CHECK=1 cxxflags=--coverage linkflags=-lgcov
    while read -a a ; do
        echo "gcov: lines: ${a[1]}  exec: ${a[2]}  cover: ${a[3]}"
        declare -i cover=${a[3]:0:-1} i=0
        # 1% = 1 line for graph extractor
        for ((i=0; i<$cover; ++i)); do
            echo 'coverage: '$i
        done
        break
    done < <(gcovr --gcov-executable $gcovbin -r . -f src/ bin/gcc*/debug/ --gcov-ignore-parse-errors | tail -n2)
    rm -r bin/gcc*

    show_duration $toolset_gcc debug

    # cppcheck
    # ./tools/c++-analyzer/cppcheck-filtered 2>&1 1>/dev/null
    # show_duration cppcheck


    # extract TODO, BUG, etc
    find \
      src \
      tests \
      projects/qtclient/src/ \
      projects/redemption_configs/ \
      \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) \
      -exec ./tools/c++-analyzer/todo_extractor '{}' +

    show_duration todo_extractor

    #set -o pipefail

    # clang analyzer
    CLANG_TIDY=clang-tidy /usr/bin/time --format="%Es - %MK" \
      ./tools/c++-analyzer/clang-tidy | sed -E '/^(.+\/|)modules\//,/\^/d'

    show_duration clang-tidy
fi

if (( $update == 1 )); then
    set -e
    # update targets.jam files
    bjam $toolset_gcc targets.jam
    cd projects/jsclient
    bjam $toolset_emscripten debug targets.jam
    cd ../redemption_configs

    # update verbose description
    desc=autogen/src/include/debug_verbose_description.hpp
    [[ -f "$desc" ]]
    ./extract_verbose.sh > "$desc".new
    cmp "$desc".new "$desc" || {
        # recompile configs
        cp ../../project-config.jam .
        mv "$desc".new "$desc"
        bjam $toolset_clang debug -j2 generate
        git add autogen
    }
    cd ../..

    # commit files
    git add targets.jam projects/jsclient/targets.jam projects/qtclient/redemption_deps.jam
    r=$(git status --ignore-submodules --porcelain | sed '/^ \?M /!d')
    [[ -z "$r" ]] || {
        git commit -m '[ci] update generated files'
        git push origin "$GIT_LOCAL_BRANCH" || {
            git reset @^
            false
        }
    }

    show_duration update
fi
