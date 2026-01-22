#!/bin/bash

##############################################################################
# @file setup.sh
# @brief Script de instalação de dependências para o DICOM Viewer
# @author DICOM Viewer Project
# @date 2026
#
# Este script detecta a distribuição Linux e instala as dependências
# necessárias para compilar o projeto (Qt6, DCMTK, CMake).
##############################################################################

set -e

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

##############################################################################
# @brief Exibe mensagem de informação
# @param $1 Mensagem a ser exibida
##############################################################################
info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

##############################################################################
# @brief Exibe mensagem de aviso
# @param $1 Mensagem a ser exibida
##############################################################################
warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

##############################################################################
# @brief Exibe mensagem de erro
# @param $1 Mensagem a ser exibida
##############################################################################
error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

##############################################################################
# @brief Detecta a distribuição Linux
# @return String com o nome da distribuição (debian, fedora, arch, unknown)
##############################################################################
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian|linuxmint|pop)
                echo "debian"
                ;;
            fedora|rhel|centos|rocky|almalinux)
                echo "fedora"
                ;;
            arch|manjaro|endeavouros)
                echo "arch"
                ;;
            opensuse*)
                echo "opensuse"
                ;;
            *)
                echo "unknown"
                ;;
        esac
    elif [ "$(uname)" == "Darwin" ]; then
        echo "macos"
    else
        echo "unknown"
    fi
}

##############################################################################
# @brief Instala dependências em sistemas Debian/Ubuntu
##############################################################################
install_debian() {
    info "Detectado sistema Debian/Ubuntu"
    info "Atualizando lista de pacotes..."
    sudo apt update

    info "Instalando dependências..."
    sudo apt install -y \
        build-essential \
        cmake \
        qt6-base-dev \
        qt6-svg-dev \
        libqt6opengl6-dev \
        libgl1-mesa-dev \
        libdcmtk-dev \
        libdcmtk17

    info "Dependências instaladas com sucesso!"
}

##############################################################################
# @brief Instala dependências em sistemas Fedora/RHEL
##############################################################################
install_fedora() {
    info "Detectado sistema Fedora/RHEL"
    info "Instalando dependências..."
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        qt6-qtbase-devel \
        qt6-qtsvg-devel \
        mesa-libGL-devel \
        dcmtk-devel

    info "Dependências instaladas com sucesso!"
}

##############################################################################
# @brief Instala dependências em sistemas Arch Linux
##############################################################################
install_arch() {
    info "Detectado sistema Arch Linux"
    info "Instalando dependências..."
    sudo pacman -S --needed \
        base-devel \
        cmake \
        qt6-base \
        qt6-svg \
        mesa \
        dcmtk

    info "Dependências instaladas com sucesso!"
}

##############################################################################
# @brief Instala dependências em sistemas openSUSE
##############################################################################
install_opensuse() {
    info "Detectado sistema openSUSE"
    info "Instalando dependências..."
    sudo zypper install -y \
        gcc-c++ \
        cmake \
        qt6-base-devel \
        qt6-svg-devel \
        Mesa-libGL-devel \
        dcmtk-devel

    info "Dependências instaladas com sucesso!"
}

##############################################################################
# @brief Instala dependências no macOS via Homebrew
##############################################################################
install_macos() {
    info "Detectado macOS"

    if ! command -v brew &> /dev/null; then
        error "Homebrew não encontrado. Por favor, instale o Homebrew primeiro:"
        echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi

    info "Instalando dependências via Homebrew..."
    brew install cmake qt@6 dcmtk

    info "Dependências instaladas com sucesso!"
    warn "Pode ser necessário adicionar Qt ao PATH:"
    echo "  export PATH=\"\$(brew --prefix qt@6)/bin:\$PATH\""
}

##############################################################################
# @brief Configura e compila o projeto
##############################################################################
build_project() {
    info "Configurando projeto com CMake..."
    cmake -B build -S .

    info "Compilando projeto..."
    cmake --build build

    info "Compilação concluída com sucesso!"
    info "Execute o programa com: ./build/dicom-visualizer"
}

##############################################################################
# @brief Função principal
##############################################################################
main() {
    echo "=============================================="
    echo "  DICOM Viewer - Script de Instalação"
    echo "=============================================="
    echo ""

    DISTRO=$(detect_distro)

    case "$DISTRO" in
        debian)
            install_debian
            ;;
        fedora)
            install_fedora
            ;;
        arch)
            install_arch
            ;;
        opensuse)
            install_opensuse
            ;;
        macos)
            install_macos
            ;;
        *)
            error "Distribuição não suportada automaticamente."
            echo ""
            echo "Por favor, instale manualmente:"
            echo "  - CMake 3.16+"
            echo "  - Qt 6 (módulos: Widgets, Svg, OpenGL, OpenGLWidgets)"
            echo "  - OpenGL development libraries (Mesa)"
            echo "  - DCMTK (biblioteca de desenvolvimento)"
            echo "  - Compilador C++ com suporte a C++17"
            exit 1
            ;;
    esac

    echo ""
    read -p "Deseja compilar o projeto agora? [S/n] " -n 1 -r
    echo ""

    if [[ $REPLY =~ ^[Ss]$ ]] || [[ -z $REPLY ]]; then
        build_project
    else
        info "Para compilar manualmente, execute:"
        echo "  cmake -B build -S ."
        echo "  cmake --build build"
    fi
}

# Executa a função principal
main "$@"
