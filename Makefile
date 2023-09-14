build:
	$ ./scripts/build.sh
PHONY: build-app

setup:
	# $ pip install conan && \
	# 	sudo apt-get install git gcc g++ build-essential cmake
	# 	# sudo apt-get install git gcc g++ libjsoncpp-dev uuid-dev zlib1g-dev build-essential cmake

	$ sudo apt update && sudo apt upgrade -y
	$ sudo apt install -y \
		curl zip unzip tar autoconf pkg-config g++ \
		make ninja-build gcc cmake git

	$ chmod +x ./scripts/build.sh
	$ chmod +x ./scripts/setup.sh

	$ ./scripts/setup.sh
PHONY: setup

run:
	$ export $(cat .env | xargs) && ./build/api-cpp-crow-exe
PHONY: run

clean:
	$ rm -fr ./build 2>/dev/null
PHONY: clean

clean-all:
	$ rm -fr ./build 2>/dev/null
	$ rm -fr ./Crow 2>/dev/null
	$ rm -fr ./vcpkg 2>/dev/null
	$ rm -fr ./vcpkg_installed 2>/dev/null
PHONY: clean

build-image:
	$ docker build --no-cache -t macedot/rinhadebackend-cpp-crow --progress=plain -f ./Dockerfile .
PHONY: build-image

start-database:
	$ docker compose -f docker-compose.yaml up -d database
PHONY: start-database

clean-database:
	$ docker volume rm rinhadebackend-cpp-crow_dbdata
PHONY: start-database

start:
	$ docker compose -f docker-compose.yaml up -d
PHONY: start

stop:
	$ docker compose -f docker-compose.yaml down
PHONY: start-database

# run-container:
# 	$ docker run --rm --name rinhadebackend-cpp-crow --env-file=.env -p 9998:9998 macedot/rinhadebackend-cpp-crow
# PHONY: run-container

# stop-container:
# 	$ docker stop rinhadebackend-cpp-crow
# PHONY: stop-container

# push-image:
# 	$ docker push macedot/rinhadebackend-cpp-crow
# PHONY: push-image
