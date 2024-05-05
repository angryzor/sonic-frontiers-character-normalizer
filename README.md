# Sonic Frontiers Character Normalizer

This Sonic Frontiers mod provides parameters and states for the extra "Another Story" characters,
so that they can also be loaded in other gamemodes.

This mod does not do anything by itself, it is meant as a base for other mods to build on.

## Setting up the development environment

You will need to have the following prerequisites installed:

* Visual Studio 2022
* CMake 3.28 or higher

Check out the project and make sure to also check out its submodules:

```sh
git clone --recurse-submodules https://github.com/angryzor/sonic-frontiers-character-normalizer.git
```

Now let CMake do its thing:

```sh
cmake -B build
```

If you have Sonic Frontiers installed in a non-standard location, you can specify that location
with the `GAME_FOLDER` variable:

```sh
cmake -B build -DGAME_FOLDER="C:\ShadowFrontiers"
```

Once CMake is finished, navigate to the build directory and open `character-normalizer.sln` with VS2022.
You should have a fully working environment available.

Building the INSTALL project will install the mod into HedgeModManager's `Mods` directory.
