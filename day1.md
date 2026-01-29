# Day 1 of the ROS Bootcamp

Class started with making sure all participants were running Ubuntu 22.04 or later. Many were running 24.04.

We quickly went over the following tutorial. It was recommended to test our local development environment instead of using Colab. VSCode with the Jupyter extension worked well.

<https://cs231n.github.io/python-numpy-tutorial/>

We then moved to the Day 1 notebook <day1/Day1_Python_Vision.ipynb>.

Note: a requirements file was added to this repository to help with the initial setup.

Here are the instructions for setting up the environment using the requirements file.

## Install Python packages in clean environment

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
```

```bash
pip install -r requirements.txt
```

Note, ipykernel was added so VSCode wouldn't prompt to install it when first running the notebook.
