# 所有执行的命令都打印到终端
set -x
# 如果执行过程中有非0退出状态，则立即退出
set -e
# 引用未定义变量则立即退出
set -u

help | head -n 1
uname

# 获取绝对路径，保证其他目录执行此脚本依然正确
pushd $(dirname "$0")
script_path=$(pwd)
popd

# 启动参数声明
debug_mode="true"
gn_clone="false"
gn_sync="false"
gn_gen="false"
gn_build="false"


# 定义平台变量
case "$(uname)" in
"Linux")
  target_os="linux"
  target_cpu="x64"
  ide=""
  ;;

"Darwin")
  target_os="mac"
  target_cpu="x64"
  ide="xcode"
  ;;

"MINGW"*|"MSYS_NT"*)
  target_os="win"
  target_cpu="x64"
  ide="vs"
  ;;
*)
  echo "Unknown OS"
  exit 1
  ;;
esac

for arg in $*                                          
do
    arg_low=$(echo $arg | tr '[:upper:]' '[:lower:]')
    if [[ $arg_low == "release" ]]; then
        debug_mode="false"
    fi
    if [[ $arg_low == "clone" ]]; then
        gn_clone="true"
    fi
    if [[ $arg_low == "sync" ]]; then
        gn_sync="true"
    fi    
    if [[ $arg_low == "gen" ]]; then
        gn_gen="true"
    fi
    if [[ $arg_low == "build" ]]; then
        gn_build="true"
    fi
done

# 提示
if [ $debug_mode == "true" ]; then
    echo current build mode: debug
else
    echo current build mode: release
fi

# 环境变量设置
depot_tools_path=$script_path/depot_tools
export PATH=$depot_tools_path:$PATH

export GYP_MSVS_VERSION=2022
export DEPOT_TOOLS_WIN_TOOLCHAIN=0

# 设置相关路径
chromium_path=$script_path/src
dispatch_path=$chromium_path/akama-sdk/out
gen_path_name=cronet-$target_os-$target_cpu
if [ $debug_mode == "true" ]; then
    gen_path_name=$gen_path_name-debug
else
    gen_path_name=$gen_path_name-release
fi

echo
echo ---------------------------------------------------------------
echo check depot_tools
echo ---------------------------------------------------------------

if [ -d $depot_tools_path ];then
    echo find $depot_tools_path
else
    echo not find $depot_tools_path, download depot_tools
    git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git

    echo
    echo ---------------------------------------------------------------
    echo gclient config
    echo ---------------------------------------------------------------

    gclient config --unmanaged --name src https://chromium.googlesource.com/chromium/src.git
fi

if [ $gn_clone == "true" ]; then
    echo
    echo ---------------------------------------------------------------
    echo clone chromium
    echo ---------------------------------------------------------------

    # clone chromium前先把akama-sdk移出去
    if [ -d $chromium_path/akama-sdk ];then
        mv $chromium_path/akama-sdk $chromium_path/../.akama-sdk
    fi
    
    #release_last_commit=$(git ls-remote https://chromium.googlesource.com/chromium/src.git --heads branch-heads/5060 | head -n 1 | cut -f 1)
    
    release_tag="103.0.5060.126"
    git -c core.deltaBaseCacheLimit=2g clone -b $release_tag --progress https://chromium.googlesource.com/chromium/src.git --depth=1 src

    pushd $chromium_path
        git apply --ignore-space-change --ignore-whitespace ../patch/akama-sdk-build.patch
    popd
fi

if [ $gn_sync == "true" ]; then
    echo
    echo ---------------------------------------------------------------
    echo gclient sync
    echo ---------------------------------------------------------------

    gclient sync --no-history    
fi

# akama-sdk在src目录中，为了使akama-sdk中的代码改动能被当前仓库识别
# 所以把chromium的.git重命名
if [ -d $chromium_path/.git ];then
    mv $chromium_path/.git $chromium_path/.bak.git
fi

# 编译前把akama-sdk移到原来位置
if [ -d $chromium_path/../.akama-sdk ];then
    mv $chromium_path/../.akama-sdk $chromium_path/akama-sdk
fi

pushd $chromium_path

if [ $gn_gen == "true" ]; then
    echo
    echo
    echo ---------------------------------------------------------------
    echo gn gen...
    echo ---------------------------------------------------------------

    # args
    args=is_debug=$debug_mode
    args=$args" target_os=\"$target_os\""
    args=$args" target_cpu=\"$target_cpu\""

    args=$args" strip_debug_info=true"

    args=$args" is_component_build=$debug_mode"
    args=$args" enable_nacl=false"

    gn gen $dispatch_path/$gen_path_name --ide=$ide --filters=akama-sdk/* --noo-deps --args="$args"
fi

if [ $gn_build == "true" ]; then
    echo
    echo
    echo ---------------------------------------------------------------
    echo ninja building...
    echo ---------------------------------------------------------------

    set NINJA_SUMMARIZE_BUILD=1
    autoninja -C $dispatch_path/$gen_path_name akama-sdk
fi

popd

echo
echo
echo ---------------------------------------------------------------
echo success!
echo ---------------------------------------------------------------
