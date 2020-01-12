
# Liste d'effets à base d'images

## Type : Simuler des volumes 3D
- [Skybox](cards/skybox.md)
- Billboards (screen-aligned ou world-oriented)
- Impostors (objet 3D rendu sur une texture (souvent sous plusieurs angles de rotation))

## Type : Post process
- Prérequis : [Rendu offscreen](cards/postprocessing.md)
- Color correction
- Tone mapping
- Blur (Gaussian and downsampling)
- Bloom
- Lens flare
- Depth of field
- Motion blur (Velocity buffer, éviter l'utilisation de l'accumulation buffer)
- Fog

## Types : Autres
- Textures animées ([voir les utilisations possible dans Mario Galaxy](https://www.youtube.com/watch?v=8rCRsOLiO7k))

# Travaux pratiques

## Le but

Ce module vous donne l'occasion de manipuler des fonctionalités essentielles du [pipeline 3D](https://en.wikipedia.org/wiki/Graphics_pipeline). L'implémentation de chacun de ces effets vous donnera un vocabulaire qui vous sera ensuite utile pour le projet de fin d'année de moteur de jeu.

## Notation

Ayant une durée d'environ une semaine, il est possible que vous n'ayez pas le temps de tous les faire. Vous devez cependant fournir une liste d'effets **____**qui justifie cette semaine de travail**____**.

## Estimation des durées d'implémentation des effets
(Sujettes à variation selon les implémentations et le degré d'approfondissement)

| Rapides            | Modérées          | Longues                           |
|--------------------|-------------------|-----------------------------------|
| Skybox             | Blur              | Impostors (si générés au runtime) |
| Billboards         | Bloom             | Motion blur                       |
| Color correction   | Fog               | Depth of field                    |
|                    | Lens flare        |                                   |

Sont pris en compte dans la notation :
- Le code de vos démo doit suivre la norme du projet et être concis/minimal. (par exemple : pénalité si du code inutile traine car vous vous êtes basé sur du code existant).
- Pas de memory (ram/vram) leaks.
- Les démos doivent avoir un rendu visuel peaufiné. (Évitez les cubes qui flottent dans les abysses).
- Le nom des variables doit être clair (dans leur contexte).
- 

# Notes et directives

La structure du projet doit être conservée car il y aura certainement des évolutions au fur et à mesure.

La norme de code, différente de d'habitude et spécifique à ce projet doit être conservée. Vous devez vous fondre dans une nouvelle base de code.

Le code utilise principalement le paradigme procédural, les abstractions sont limitées au maximum afin de toujours être au plus bas niveau de l'interface OpenGL. 

Chaque effet doit être implémenté dans un fichier `demo_xxx.cpp` avec l'entête équivalent.

Les seules choses à modifier dans le `main.cpp` sont les endroits marqués par `TODO(demo)`.

```c++
#include "demo_base.h"
// TODO(demo): Add headers here
#include "demo_xxx.h"
```
et
```c++
std::unique_ptr<demo> Demos[] = 
{
    std::make_unique<demo_base>(),
    // TODO(demo): Add other demos here
    std::make_unique<demo_xxx>(),
};
```

Le fichier `maths.h` est volontairement limité, il va potentiellement être mis à jour au cours du projet. Ajoutez les fonctions dont vous avez besoin dans `maths_extension.h` (inclus dans maths.h) pour éviter d'éventuels conflits.

# Description des fichiers

[```demo.h```](src/demo.h) :
- Interface à implémenter pour chacun des effets.

[```main.cpp```](src/main.cpp) : 
- S'occupe de la création de la fenêtre et du contexte OpenGL.
- Instancie toutes les démos.
- Contient la boucle principale, qui récupère les inputs et communique avec les démos grâce à ```platform_io```.
- (C'est le seul fichier à réécrire  si on voulait porter le projet sous une autre plateforme non gérée par GLFW)

[```platform.h```](src/platform.h) :
- Contient ```platform_io``` qui décrit les données de base (dimensions d'écran, mouvement de la souris) nécessairent à .

[```demo_minimal.cpp```](src/demo_minimal.cpp) :
- Exemple minimal de l'interface de démo.

[```demo_postprocess.cpp```](src/demo_minimal.cpp) :
- Exemple de rendu hors screen (1ère passe) afin de modifier les couleurs lors d'une 2ème passe.

[```types.h```](src/types.h) :
- Types primitifs vecteurs/matrices : ```v2```, ```v3```, ```v4``` et ```mat4```.

[```maths.h```](src/maths.h) :
- Fonctions mathématiques et surcharges d'opérateurs pour les types ```v2```, ```v3```, ```v4``` et ```mat4```.
- Utilisez le fichier ```maths_extension.h``` si vous avez besoin d'ajouter vos propres fonctions.

[```camera.h```](src/camera.h) :
- Gestion des déplacement (mode FPS ou libre)

[```mesh.h```](src/mesh.h) :
- Outil de construction de mesh.
- Permet de charger les types primitifs (quad, cube, sphere) et les models obj dans vos propres format de vertex.


``` c++
// Create vertex format descriptor
vertex_descriptor Descriptor = {};
Descriptor.Stride = sizeof(vertex);
Descriptor.PositionOffset = offsetof(vertex, Position);

// Create a cube in RAM
vertex Cube[36];
this->VertexCount = 36;
Mesh::BuildCube(Cube, Cube + this->VertexCount, Descriptor);
```

```opengl_headers.h``` : Entêtes OpenGL.

```opengl_helpers.h``` : Divers outils pour OpenGL
- ```class GL::debug``` : Affichage wireframe d'un vbo.
- ```class GL::cache``` : Permet d'accélérer les chargements des .obj et textures.
- fonction ```GL::CreateProgram()``` : Compilation du shader avec options d'injecter une fonction de shading de type phong.
- fonction ```GLImGui::InspectProgram``` : Permet d'inspecter un shader et notamment de modifier les sources et les uniforms à la volée.

```color.h``` :
- Fonctions de conversion de code couleur en ```v3```/```v4```.

## Liens utiles
- Specs OpenGL 3.3 Core : https://www.khronos.org/registry/OpenGL/specs/gl/glspec33.core.pdf
- Livre référence + Liste de ressources liées au rendu : https://www.realtimerendering.com/
- Documentation OpenGL : http://docs.gl/
- Analyse de frames de divers jeux : http://www.adriancourreges.com/projects/
- Exploration WebGL de jeux Nintendo : https://noclip.website/ et analyse des techniques utilisées https://www.youtube.com/watch?v=8rCRsOLiO7k
- Uncharted 2: HDR Lighting : https://www.gdcvault.com/play/1012351/Uncharted-2-HDR
- RenderDoc, outil d'analyse de frames (utile pour debugger sa propre application) : https://renderdoc.org/
- GLIntercept, permet entre autre de mettre en évidence les memory leaks : https://github.com/dtrebilco/glintercept 

## Assets
- 3d model *Fantasy Game Inn* by [**pepedrago**](https://sketchfab.com/pepedrago): https://sketchfab.com/3d-models/fantasy-game-inn-192bf30a7e28425ab385aef19769d4b0
- *Generic Night Skybox* by **JasperCarmack**: https://gamebanana.com/textures/download/5345

## Bibliothèques externes
- [```nothings/stb```]() pour le chargement des images.
- [```ocornut/imgui```](https://github.com/ocornut/imgui) pour l'affichage de l'UI.
- [```syoyo/tiny_obj_loader```](https://github.com/syoyo/tinyobjloader) pour le chargement des models .obj.
