# PocketCHIP Launcher (Marshmallow Edition)

## Required Packages

```sh
sudo apt-get install \
    git \
    build-essential \
    libx11-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxft-dev \
    libxinerama-dev \
    libnm-glib-dev \
    libnm-gtk-dev \
    network-manager-dev
```

## Building
```
git clone --recursive https://github.com/o-marshmallow/PocketCHIP-pocket-home/
make
make devinstall
sudo systemctl restart lightdm
```

## Updating
```
git pull
git submodule update
make
make devinstall
sudo systemctl restart lightdm
```

### Thanks
Big thanks to [Celti](https://github.com/Celti) who made this repo updated to new JUCE version and easier to compile
