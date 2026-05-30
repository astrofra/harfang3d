#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
	echo "Ce script cible macOS. Pour Windows, utilise rebuild_hg_ffmpeg.bat."
	exit 1
fi

CONFIG="${1:-Release}"

REPO_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$(cd -- "${REPO_DIR}/.." && pwd)"

BUILD_DIR="${BUILD_DIR:-${WORK_DIR}/build/ffmpeg-plugin}"
INSTALL_DIR="${INSTALL_DIR:-${WORK_DIR}/install/ffmpeg-plugin}"

if [[ -z "${FABGEN_DIR:-}" ]]; then
	if [[ -d "${WORK_DIR}/FABGen" ]]; then
		FABGEN_DIR="${WORK_DIR}/FABGen"
	elif [[ -d "${WORK_DIR}/fabgen" ]]; then
		FABGEN_DIR="${WORK_DIR}/fabgen"
	fi
fi

if [[ -n "${2:-}" ]]; then
	FFMPEG_ROOT="${2}"
fi

if [[ -z "${FFMPEG_ROOT:-}" ]]; then
	if [[ -d "${WORK_DIR}/ffmpeg" ]]; then
		FFMPEG_ROOT="${WORK_DIR}/ffmpeg"
	elif [[ -d "/opt/homebrew/opt/ffmpeg" ]]; then
		FFMPEG_ROOT="/opt/homebrew/opt/ffmpeg"
	elif [[ -d "/usr/local/opt/ffmpeg" ]]; then
		FFMPEG_ROOT="/usr/local/opt/ffmpeg"
	elif command -v brew >/dev/null 2>&1; then
		BREW_FFMPEG_ROOT="$(brew --prefix ffmpeg 2>/dev/null || true)"
		if [[ -n "${BREW_FFMPEG_ROOT}" && -d "${BREW_FFMPEG_ROOT}" ]]; then
			FFMPEG_ROOT="${BREW_FFMPEG_ROOT}"
		fi
	fi
fi

if [[ -z "${FABGEN_DIR:-}" || ! -d "${FABGEN_DIR}" ]]; then
	echo "FABGen introuvable: \"${FABGEN_DIR:-${WORK_DIR}/FABGen}\""
	echo "Clone FABGen dans ce dossier ou adapte FABGEN_DIR dans ce script."
	exit 1
fi

if [[ -z "${FFMPEG_ROOT:-}" ]]; then
	echo "FFmpeg SDK introuvable."
	echo "Definis FFMPEG_ROOT ou passe son chemin en deuxieme argument:"
	echo "  rebuild_hg_ffmpeg.sh ${CONFIG} /path/to/ffmpeg"
	exit 1
fi

if [[ ! -f "${FFMPEG_ROOT}/include/libavformat/avformat.h" ]]; then
	echo "Headers FFmpeg introuvables dans \"${FFMPEG_ROOT}/include\"."
	echo "Verifie FFMPEG_ROOT: \"${FFMPEG_ROOT}\""
	exit 1
fi

if [[ ! -f "${FFMPEG_ROOT}/lib/libavformat.dylib" && ! -f "${FFMPEG_ROOT}/lib/libavformat.a" ]]; then
	echo "Librairies FFmpeg introuvables dans \"${FFMPEG_ROOT}/lib\"."
	echo "Verifie FFMPEG_ROOT: \"${FFMPEG_ROOT}\""
	exit 1
fi

if [[ -z "${PYTHON_EXE:-}" ]]; then
	if command -v python3 >/dev/null 2>&1; then
		PYTHON_EXE="$(python3 -c 'import sys; print(sys.executable)')"
	elif command -v python >/dev/null 2>&1; then
		PYTHON_EXE="$(python -c 'import sys; print(sys.executable if sys.version_info[0] >= 3 else "")' 2>/dev/null || true)"
	fi
fi

if [[ -z "${PYTHON_EXE:-}" ]]; then
	echo "Python 3 introuvable. Definis PYTHON_EXE ou installe Python 3 dans le PATH."
	exit 1
fi

if [[ -z "${CMAKE_GENERATOR:-}" ]]; then
	if command -v ninja >/dev/null 2>&1; then
		CMAKE_GENERATOR="Ninja"
	else
		CMAKE_GENERATOR="Unix Makefiles"
	fi
fi

mkdir -p "${BUILD_DIR}"

echo "[1/3] Configuration CMake..."
cmake -S "${REPO_DIR}" -B "${BUILD_DIR}" -G "${CMAKE_GENERATOR}" \
	-DCMAKE_BUILD_TYPE="${CONFIG}" \
	-DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
	-DHG_FABGEN_PATH="${FABGEN_DIR}" \
	-DPython3_EXECUTABLE="${PYTHON_EXE}" \
	-DFFMPEG_ROOT="${FFMPEG_ROOT}" \
	-DHG_BUILD_FFMPEG_PLUGIN=ON \
	-DHG_BUILD_CPP_SDK=OFF \
	-DHG_BUILD_TESTS=OFF \
	-DHG_BUILD_DOCS=OFF \
	-DHG_BUILD_STATIC_DOCS=OFF \
	-DHG_BUILD_HG_LUA=OFF \
	-DHG_BUILD_HG_PYTHON=OFF \
	-DHG_BUILD_HG_GO=OFF \
	-DHG_BUILD_ASSETC=OFF \
	-DHG_BUILD_ASSIMP_CONVERTER=OFF \
	-DHG_BUILD_FBX_CONVERTER=OFF \
	-DHG_BUILD_GLTF_IMPORTER=OFF \
	-DHG_BUILD_GLTF_EXPORTER=OFF

echo "[2/3] Build hg_ffmpeg (${CONFIG})..."
build_cmd=(cmake --build "${BUILD_DIR}" --target hg_ffmpeg)
if grep -q '^CMAKE_CONFIGURATION_TYPES:' "${BUILD_DIR}/CMakeCache.txt"; then
	build_cmd+=(--config "${CONFIG}")
fi
"${build_cmd[@]}"

echo "[3/3] Installation du plugin..."
install_cmd=(cmake --install "${BUILD_DIR}" --component cppsdk)
if grep -q '^CMAKE_CONFIGURATION_TYPES:' "${BUILD_DIR}/CMakeCache.txt"; then
	install_cmd+=(--config "${CONFIG}")
fi
"${install_cmd[@]}"

plugin_candidates=(
	"${INSTALL_DIR}/cppsdk/bin/${CONFIG}/hg_ffmpeg.dylib"
	"${INSTALL_DIR}/cppsdk/bin/hg_ffmpeg.dylib"
)
PLUGIN_DYLIB=""
for candidate in "${plugin_candidates[@]}"; do
	if [[ -f "${candidate}" ]]; then
		PLUGIN_DYLIB="${candidate}"
		break
	fi
done

if [[ -z "${PLUGIN_DYLIB}" ]]; then
	echo "Dylib du plugin introuvable dans l'installation CMake."
	exit 1
fi

FINAL_DIR="${INSTALL_DIR}/${CONFIG}"
mkdir -p "${FINAL_DIR}"
cp -f "${PLUGIN_DYLIB}" "${FINAL_DIR}/"

# Miroir du script Windows: depose aussi les dylibs FFmpeg a cote du plugin
# pour les tests locaux.
shopt -s nullglob
ffmpeg_runtime_libs=(
	"${FFMPEG_ROOT}/lib/libavcodec"*.dylib
	"${FFMPEG_ROOT}/lib/libavdevice"*.dylib
	"${FFMPEG_ROOT}/lib/libavformat"*.dylib
	"${FFMPEG_ROOT}/lib/libavutil"*.dylib
	"${FFMPEG_ROOT}/lib/libswresample"*.dylib
	"${FFMPEG_ROOT}/lib/libswscale"*.dylib
)
if (( ${#ffmpeg_runtime_libs[@]} > 0 )); then
	cp -f "${ffmpeg_runtime_libs[@]}" "${FINAL_DIR}/"
fi
shopt -u nullglob

echo
echo "FFmpeg plugin rebuild ok."
echo "Plugin: \"${FINAL_DIR}/hg_ffmpeg.dylib\""
echo "Dylibs: \"${FINAL_DIR}\""
