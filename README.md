All of the project for Towers of Hanoi,
was done in equal parts by Danny To (100586781) and Rishabh Patel (100583380).

Towers of Hanoi project consist of the following:
  - Parametrically generated cylinder
  - Models for Torus(the rings), and Cube(for the base)
  - Another model for the Skybox was also generated using blender,
      Blender was used to Flip the normals, so the texture for the skybox
      could be applied to the interior.
      (Texture used for the skybox was taken from:
        https://medium.com/level-up-web/free-packs-of-hi-res-backgrounds-textures-ae98fb0febb4)
  - CreateTexture function used was taken from a lab


  - The Animations for solving Towers of Hanoi were done using Key-Frame Animation

  - A single key press(interaction) on "L" will allow the light to rotate by itself
  
  - A video of Building and Running the application can be found here: https://youtu.be/6sgtcw-ki3Y


# Windows Only

- Open: Visual Studio Command Prompt and change directory to project folder:

- To build the project;

    - Enter the cmd:
    > nmake /f Nmakefile.Windows


- To run the project

    - Enter the cmd:
    > main
