#!/usr/bin/env bash
set -euo pipefail

CONFIG="${1:-Release}"

REPO_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$(cd -- "${REPO_DIR}/.." && pwd)"

BUILD_DIR="${BUILD_DIR:-${WORK_DIR}/build/lua-cmake}"
INSTALL_DIR="${INSTALL_DIR:-${WORK_DIR}/install/lua}"
FABGEN_VENV_DIR="${FABGEN_VENV_DIR:-${WORK_DIR}/build/fabgen-venv}"

if [[ -z "${FABGEN_DIR:-}" ]]; then
	if [[ -d "${WORK_DIR}/FABGen" ]]; then
		FABGEN_DIR="${WORK_DIR}/FABGen"
	elif [[ -d "${WORK_DIR}/fabgen" ]]; then
		FABGEN_DIR="${WORK_DIR}/fabgen"
	fi
fi

if [[ -z "${FABGEN_DIR:-}" || ! -d "${FABGEN_DIR}" ]]; then
	echo "FABGen introuvable: \"${FABGEN_DIR:-${WORK_DIR}/FABGen}\""
	echo "Clone FABGen dans ce dossier ou adapte FABGEN_DIR dans ce script."
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

BOOTSTRAP_PYTHON_EXE="${PYTHON_EXE}"
VENV_PYTHON_EXE="${FABGEN_VENV_DIR}/bin/python"

if [[ ! -x "${VENV_PYTHON_EXE}" ]]; then
	echo "[prep] Creation du venv FABGen: \"${FABGEN_VENV_DIR}\""
	"${BOOTSTRAP_PYTHON_EXE}" -m venv "${FABGEN_VENV_DIR}"
fi

if ! "${VENV_PYTHON_EXE}" -c 'import pypeg2' >/dev/null 2>&1; then
	echo "[prep] Installation de la dependance FABGen: pypeg2"
	PIP_USER=0 "${VENV_PYTHON_EXE}" -m pip install --no-user --upgrade pip
	PIP_USER=0 "${VENV_PYTHON_EXE}" -m pip install --no-user pypeg2
fi

PYTHON_EXE="${VENV_PYTHON_EXE}"

if [[ -z "${CMAKE_GENERATOR:-}" ]]; then
	if command -v ninja >/dev/null 2>&1; then
		CMAKE_GENERATOR="Ninja"
	else
		CMAKE_GENERATOR="Unix Makefiles"
	fi
fi

mkdir -p "${BUILD_DIR}"

echo "[1/2] Configuration CMake..."
cmake -S "${REPO_DIR}" -B "${BUILD_DIR}" -G "${CMAKE_GENERATOR}" \
	-DCMAKE_BUILD_TYPE="${CONFIG}" \
	-DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
	-DHG_FABGEN_PATH="${FABGEN_DIR}" \
	-DPython3_EXECUTABLE="${PYTHON_EXE}" \
	-DHG_BUILD_HG_LUA=ON \
	-DHG_BUILD_ASSETC=ON \
	-DHG_BUILD_ASSIMP_CONVERTER=OFF \
	-DHG_BUILD_FBX_CONVERTER=OFF \
	-DHG_BUILD_GLTF_IMPORTER=OFF \
	-DHG_BUILD_GLTF_EXPORTER=OFF

echo "[2/2] Build et install HG Lua + AssetC (${CONFIG})..."
build_cmd=(cmake --build "${BUILD_DIR}" --target install)
if grep -q '^CMAKE_CONFIGURATION_TYPES:' "${BUILD_DIR}/CMakeCache.txt"; then
	build_cmd+=(--config "${CONFIG}")
fi
"${build_cmd[@]}"

assetc_install_script="${BUILD_DIR}/tools/assetc/cmake_install.cmake"
if [[ ! -f "${assetc_install_script}" ]]; then
	echo "Script d'installation AssetC introuvable: \"${assetc_install_script}\""
	exit 1
fi

cmake \
	-DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}/hg_lua/harfang" \
	-DCMAKE_INSTALL_CONFIG_NAME="${CONFIG}" \
	-DBUILD_TYPE="${CONFIG}" \
	-P "${assetc_install_script}"

echo
echo "HG Lua + AssetC rebuild ok."
echo "Install: \"${INSTALL_DIR}/hg_lua\""
