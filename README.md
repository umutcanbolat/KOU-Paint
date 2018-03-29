# KOU Paint [![Build Status](https://travis-ci.org/umutcanbolat/KOU-Paint.svg?branch=master)](https://travis-ci.org/umutcanbolat/KOU-Paint)

KOU Paint is a lightweight image editor that helps users edit&save image files quickly and supports JPEG, PNG, BMP, PCX and TGA formats. 

Developed in C using allegro5 library.


## Installation

Install `pkg-config`, `make` and `gcc` to your system then run these commands in the application directory.

### For Ubuntu
```sh
sudo add-apt-repository ppa:allegro/5.2
sudo apt-get update
sudo apt-get install liballegro5-dev
make && make run
```
### For Arch Linux
```sh
sudo pacman -S pkg-config allegro
make && make run
```

## Key Bindings
| Key      | Function |
| ---------| -------- |
| CTRL + O |  Open File|
| CTRL + S |  Save |
| CTRL + Z |  Undo |
| CTRL + Y |  Redo |
| CTRL + Q |  Quit |


## Screenshots
- Blank Screen <br> <img src="https://user-images.githubusercontent.com/10065235/38103267-9c2d0490-338e-11e8-9895-c0583a77ab46.png" width="500" height="350"> <br>
- Drawing Mod <br> <img src="https://user-images.githubusercontent.com/10065235/38103451-04978b4a-338f-11e8-9db7-0bd97a3842b7.png" width="500" height="350">  <br>
- Triangle Mod <br> <img src="https://user-images.githubusercontent.com/10065235/38103463-0c07dff6-338f-11e8-9db7-ffa549bf8814.png" width="500" height="350"> <br>
- Line Mod <br> <img src="https://user-images.githubusercontent.com/10065235/38103472-11dbfaf2-338f-11e8-98af-c9f941b8ca21.png" width="500" height="350"> <br>
- Rectangle Mod <br> <img src="https://user-images.githubusercontent.com/10065235/38103485-1a5ecc40-338f-11e8-9b23-64ae095e1367.png" width="500" height="350">  <br>
- Circle Mod <br> <img src="https://user-images.githubusercontent.com/10065235/38103499-226b3630-338f-11e8-8b8d-7ab8831058a1.png" width="500" height="350"> <br>

## Credits
Design and programming by [Umut Canbolat](https://github.com/umutcanbolat) and [Ali Şentaş](https://github.com/alisentas).
