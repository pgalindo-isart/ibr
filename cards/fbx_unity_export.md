
# Materials fix
Blender script : Change material slot link to 'OBJECT' instead of data

```python
import bpy
scene = bpy.context.scene

for obj in scene.objects:
    for materialSlot in obj.material_slots:
        prevMat = materialSlot.material
        materialSlot.link = 'OBJECT'
        materialSlot.material = prevMat
```

Blender script : Replace all broken walls ('Wall_Bedroom') by 'Wall_Corridor'
```python
import bpy

scene = bpy.context.scene

for obj in scene.objects:
    for materialSlot in obj.material_slots:
        if materialSlot.material.name == 'Wall_Bedroom':
            materialSlot.material = bpy.data.materials['Wall_Corridor']
```

Then replace manually missing materials...

# Convert .tif to .png

Blender > File > External Data > Pack All Into .blend

Blender > File > External Data > Unpack All Into Files

Using Image Magick, in the texture folder :
```bash
find . -name "*.tif" -exec convert {} {}.png \;
rename .tif.png .png *.png
rename .tif-0.png .png *.png
```

```python
import bpy

for img in bpy.data.images:
    img.filepath = img.filepath.replace('tif', 'png')
    img.file_format = 'PNG'
    img.reload()
``` 

# Export to embedded .fbx

Blender > File > External Data > Make All Path Absolute

Blender > Export > FBX (.fbx)

Using option 'Path Mode = Copy + Embed'
