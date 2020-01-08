
# List of effects to implement

## FAKING 3D
- Skybox
- Billboard (screen-aligned and world-oriented)
- Impostors (render a 3d object to billboard)
- Bonus: LOD + Impostors to display a LOT of objects

## POST PROCESS
- Color correction
- Tone mapping
- Blur (Gaussian and downsampling)
- Bloom
- Depth of field
- Motion blur (Accumulation buffer, Velocity buffer)
- Fog

## MORE TO COME

# Notes et directives

La structure du projet doit être conservée car il y aura certainement des évolutions au fur et à mesure.

La norme de code, différente de d'habitude et spécifique à ce projet doit être conservée. Vous devez vous fondre dans une nouvelle base de code.

Le code utilise principalement le paradigme procédural, cela permet d'avoir une vision claire des appels OpenGL.

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

Liens utiles : 
- https://www.realtimerendering.com/
- http://docs.gl/
- https://noclip.website/ et https://www.youtube.com/watch?v=8rCRsOLiO7k