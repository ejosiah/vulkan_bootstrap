"# Vulkan bootstrap g8 template

# create a new project
Run the command below from root project level

`g8 file://project_template.g8/ --name=project_name --classname=className --title="window title" --render=yes --compute=no --raytracing=no`

# installing giter8

## windows installation
first install coursier by running this on the command line:

`bitsadmin /transfer cs-cli https://git.io/coursier-cli-windows-exe "%cd%\cs.exe"`

run `cs --help` to confirm that the installation was successful

now install giter 8 by running this on the command line:

`cs install giter8`

## Linux & macOS
first install coursier by running these on the command line:

`curl -fLo cs https://git.io/coursier-cli-"$(uname | tr LD ld)"`

`chmod +x cs`

`rm cs`

Alternatively on macOS if you have brew installed then you can install coursier by running this:

`brew install coursier/formulas/coursier`

now install giter 8 by running this on the command line:

`cs install giter8`