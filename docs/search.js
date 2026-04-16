window.HG_SEARCH_INDEX = [
  {
    "title": "Overview",
    "href": "manual/overview.html",
    "kind": "manual",
    "text": "Harfang is a high-level software library to create applications that display 2D/3D visuals and play sound/music. It provides a unified API to write programs using different programming languages and is available for Windows and Linux. For m"
  },
  {
    "title": "Requirements",
    "href": "manual/requirements.html",
    "kind": "manual",
    "text": "## System Requirements * **GPU:** Graphic card with OpenGL 3.3, Direct3D 11 or OpenGL ES 3.1 support. * **CPU** and **memory** requirements are mostly dependent on your project characteristics. A faster computer for development is recommend"
  },
  {
    "title": "Harfang for CPython",
    "href": "manual/cpython.html",
    "kind": "manual",
    "text": "Harfang for Python is distributed as a wheel package `(.whl)` compatible with official builds of the CPython interpreter version 3.4 or newer. ## Installation * On Windows, install from the command line by typing `pip install harfang`. * On"
  },
  {
    "title": "Harfang for Lua",
    "href": "manual/lua.html",
    "kind": "manual",
    "text": "Harfang for Lua is distributed as a dynamic library compatible with official builds of the Lua interpreter version 5.3. ## Installation The mecanisms used by the Lua interpreter to locate binary extensions are detailed in the [Lua 5.3 Refer"
  },
  {
    "title": "Quickstart",
    "href": "manual/quickstart.html",
    "kind": "manual",
    "text": "Once you have a functioning installation of Harfang for your language of choice: * man.CPython * man.Lua Follow the following steps: 1. Download the tutorials from Github [here](https://github.com/harfang3d/tutorials-hg2.git) and unzip them"
  },
  {
    "title": "Resources & Assets",
    "href": "manual/assets.html",
    "kind": "manual",
    "text": "By convention, production files are called **resources** (eg. *the project resources*). Files issued from the compilation of production files for a specific target are called **assets** (eg. *the project assets for Windows PC*). TOC ## Reso"
  },
  {
    "title": "Compiling to Assets",
    "href": "manual/assetcompiler.html",
    "kind": "manual",
    "text": "Compiling project resources into assets is done using the `assetc` command-line tool. Upon invocation, it will scan the input folder and compile all resources in a supported format to the output folder. Files in an unsupported format are co"
  },
  {
    "title": "Importing from GLTF",
    "href": "manual/gltf.html",
    "kind": "manual",
    "text": "The GLTF importer is a command-line tool to convert a GLTF file to Harfang resources. This tool supports converting scene graph, geometries, materials and animations. TOC ## Command-Line ```text gltf-import [-out PATH] [-base-resource-path "
  },
  {
    "title": "Importing from FBX",
    "href": "manual/fbx.html",
    "kind": "manual",
    "text": "The FBX converter is a command-line tool to convert an FBX file to Harfang resources. This tool supports converting scene graph, geometries, materials and animations. TOC ## Command-Line ```text fbx-convert [-out PATH] [-test-import] [-base"
  },
  {
    "title": "Importing from Assimp",
    "href": "manual/assimp.html",
    "kind": "manual",
    "text": "The Assimp converter is a command-line tool to convert an Assimp file to Harfang resources. This tool supports converting scene graph, geometries, materials and animations. TOC ## Command-Line ```text assimp_convert [-out PATH] [-test-impor"
  },
  {
    "title": "Reading Input",
    "href": "manual/input.html",
    "kind": "manual",
    "text": "The input system provides access to the HID devices connected to the host machine. ## Supported Device Classes All supported input device classes are utilized in the same manner. All devices connected to the host are identified by a unique "
  },
  {
    "title": "Playing Audio",
    "href": "manual/audio.html",
    "kind": "manual",
    "text": "The audio system can play back two kind of audio resource, sound and stream. * A sound resource is played from memory, this is usually the most efficient resource to play. However, since it must fully be stored in memory longer audio sample"
  },
  {
    "title": "Writing a Shader",
    "href": "manual/shader.html",
    "kind": "manual",
    "text": "TOC ## Shader Language Overview Harfang uses [bgfx](https://bkaradzic.github.io/bgfx/index.html) as its rendering system, the cross-platform shader language is based on [GLSL](https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.4"
  },
  {
    "title": "Writing a Pipeline Shader",
    "href": "manual/pipelineshader.html",
    "kind": "manual",
    "text": "A pipeline shader solves the problem of having a single shader source code accepting optional parameters. For example, a pipeline shader might accept a diffuse texture *or* a diffuse color. Having this level of flexibility in a single shade"
  },
  {
    "title": "Drawing to Views",
    "href": "manual/views.html",
    "kind": "manual",
    "text": "Views are an essential mechanism of the 3d rendering backend and must be thoroughly understood to perform any form of complex rendering. ## Overview Views are refered to by index as an integer value between 0 and 255. There can be no more t"
  },
  {
    "title": "Using the Forward Pipeline",
    "href": "manual/forwardpipeline.html",
    "kind": "manual",
    "text": "The forward pipeline implements multi-pass drawing of model batches. TOC ## Features - Separate opaque/transparent passes. - 8 light slots: 1 linear, 3 spot/point and 4 point lights. - PBR support. ## Pipeline Shaders This pipeline comes wi"
  },
  {
    "title": "Ownership & References",
    "href": "manual/ownership.html",
    "kind": "manual",
    "text": "Many resource types are returned by value or as generational reference to your program. A generational reference is a weak pointer to a resource. It has no control over the resource lifetime but can be queried for its availability. Using a "
  },
  {
    "title": "Coordinates and Units System",
    "href": "manual/coordinateandunitsystem.html",
    "kind": "manual",
    "text": "Harfang uses a left-handed coordinate system with the X axis pointing right, the Y axis pointing up and the Z axis pointing away from the viewer. For units, it uses the International System of Units (SI) or metric system: - Distances are ex"
  },
  {
    "title": "Working with Scene",
    "href": "manual/scene.html",
    "kind": "manual",
    "text": "A scene is a 3d world populated with Node. Nodes are container objects taking meaning through the use of components. ## Node & Components Calling Scene_CreateNode returns an empty node with no component attached. In this state, it serves li"
  },
  {
    "title": "Drawing a Scene",
    "href": "manual/drawingscene.html",
    "kind": "manual",
    "text": "Drawing a scene is done by calling SubmitSceneToPipeline. This function expects a start view index and may use multiple views to complete its operation. It returns an object mapping pipeline stages to view indices and the next unused view i"
  },
  {
    "title": "Physics",
    "href": "manual/physics.html",
    "kind": "manual",
    "text": "Use physics to enable physically plausible interactions with your scene content. Different backends are supported such as Bullets Physics, NVIDIA PhysX and Newton Dynamics. TOC ## Declaring Physics To turn a node into a rigid body, create a"
  },
  {
    "title": "Scripting",
    "href": "manual/scripting.html",
    "kind": "manual",
    "text": "Scripts can be used to extend the behavior of nodes and scenes. TOC ## Host vs. Embedded VM When using Harfang from a scripting language it can be difficult to differentiate between parts of your program running on you main script VM and pa"
  },
  {
    "title": "Navigation",
    "href": "manual/navigation.html",
    "kind": "manual",
    "text": "The navigation system is used to determine how to go from one world position to another as a serie of waypoints. ## Navigation Mesh In order to perform a navigation query you need a navigation mesh. A navigation mesh is compiled by the asse"
  },
  {
    "title": "Virtual Reality",
    "href": "manual/vr.html",
    "kind": "manual",
    "text": "Virtual reality is supported through the OpenVR API on the Windows operating system. TOC ## Prerequisites - A compatible VR headset (HTC Vive, Occulus Rift or Mixed Reality compatible). - SteamVR must be installed on your computer. ## OpenV"
  },
  {
    "title": "Bloom",
    "href": "api/cpython/classes.html#bloom",
    "kind": "class",
    "text": "Bloom post-process object holding internal states and resources. Create with CreateBloomFromAssets or CreateBloomFromFile, use with ApplyBloom, finally call DestroyBloom to dispose of resources when done."
  },
  {
    "title": "btGeneric6DofConstraint",
    "href": "api/cpython/classes.html#btgeneric6dofconstraint",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Camera",
    "href": "api/cpython/classes.html#camera",
    "kind": "class",
    "text": "Add this component to a Node to implement the camera aspect. Create a camera component with Scene_CreateCamera, use CreateCamera to create a complete camera node."
  },
  {
    "title": "CameraZRange",
    "href": "api/cpython/classes.html#camerazrange",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Canvas",
    "href": "api/cpython/classes.html#canvas",
    "kind": "class",
    "text": "Holds the canvas properties of a scene, see the `canvas` member of class Scene."
  },
  {
    "title": "Collision",
    "href": "api/cpython/classes.html#collision",
    "kind": "class",
    "text": "Collision component, see man.Physics."
  },
  {
    "title": "Color",
    "href": "api/cpython/classes.html#color",
    "kind": "class",
    "text": "Four-component RGBA color object."
  },
  {
    "title": "ColorList",
    "href": "api/cpython/classes.html#colorlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Contact",
    "href": "api/cpython/classes.html#contact",
    "kind": "class",
    "text": "Object containing the world space position, normal and depth of a contact as reported by the collision system."
  },
  {
    "title": "ContactList",
    "href": "api/cpython/classes.html#contactlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Crowd",
    "href": "api/cpython/classes.html#crowd",
    "kind": "class",
    "text": "A group of navigation agents to efficiently simulate a crowd, see man.Navigation."
  },
  {
    "title": "CrowdAgent",
    "href": "api/cpython/classes.html#crowdagent",
    "kind": "class",
    "text": "State of a crowd agent."
  },
  {
    "title": "CrowdAgentParams",
    "href": "api/cpython/classes.html#crowdagentparams",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Data",
    "href": "api/cpython/classes.html#data",
    "kind": "class",
    "text": ""
  },
  {
    "title": "DearImguiContext",
    "href": "api/cpython/classes.html#dearimguicontext",
    "kind": "class",
    "text": "Context to render immediate GUI."
  },
  {
    "title": "DirEntry",
    "href": "api/cpython/classes.html#direntry",
    "kind": "class",
    "text": ""
  },
  {
    "title": "DirEntryList",
    "href": "api/cpython/classes.html#direntrylist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "draw_sceneCallback",
    "href": "api/cpython/classes.html#draw_scenecallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Environment",
    "href": "api/cpython/classes.html#environment",
    "kind": "class",
    "text": "Environment properties of a scene, see `environment` member of the Scene class."
  },
  {
    "title": "File",
    "href": "api/cpython/classes.html#file",
    "kind": "class",
    "text": "Interface to a file on the host local filesystem."
  },
  {
    "title": "FileFilter",
    "href": "api/cpython/classes.html#filefilter",
    "kind": "class",
    "text": ""
  },
  {
    "title": "FileFilterList",
    "href": "api/cpython/classes.html#filefilterlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Font",
    "href": "api/cpython/classes.html#font",
    "kind": "class",
    "text": "Font object for realtime rendering."
  },
  {
    "title": "ForwardPipeline",
    "href": "api/cpython/classes.html#forwardpipeline",
    "kind": "class",
    "text": "Rendering pipeline implementing a forward rendering strategy. The main characteristics of this pipeline are: - Render in two passes: opaque display lists then transparent ones. - Fixed 8 light slots supporting 1 linear light with PSSM shado"
  },
  {
    "title": "ForwardPipelineAAA",
    "href": "api/cpython/classes.html#forwardpipelineaaa",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ForwardPipelineAAAConfig",
    "href": "api/cpython/classes.html#forwardpipelineaaaconfig",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ForwardPipelineFog",
    "href": "api/cpython/classes.html#forwardpipelinefog",
    "kind": "class",
    "text": "Fog properties for the forward pipeline."
  },
  {
    "title": "ForwardPipelineLight",
    "href": "api/cpython/classes.html#forwardpipelinelight",
    "kind": "class",
    "text": "Single light for the forward pipeline. The complete lighting rig is passed as a ForwardPipelineLights, see PrepareForwardPipelineLights."
  },
  {
    "title": "ForwardPipelineLightList",
    "href": "api/cpython/classes.html#forwardpipelinelightlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ForwardPipelineLights",
    "href": "api/cpython/classes.html#forwardpipelinelights",
    "kind": "class",
    "text": ""
  },
  {
    "title": "FrameBuffer",
    "href": "api/cpython/classes.html#framebuffer",
    "kind": "class",
    "text": ""
  },
  {
    "title": "FrameBufferHandle",
    "href": "api/cpython/classes.html#framebufferhandle",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Frustum",
    "href": "api/cpython/classes.html#frustum",
    "kind": "class",
    "text": "A view frustum, perspective or orthographic, holding the necessary information to perform culling queries. It can be used to test wether a volume is inside or outside the frustum it represents."
  },
  {
    "title": "Gamepad",
    "href": "api/cpython/classes.html#gamepad",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetGamepadNames to query for available gamepad devices."
  },
  {
    "title": "GamepadState",
    "href": "api/cpython/classes.html#gamepadstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Geometry",
    "href": "api/cpython/classes.html#geometry",
    "kind": "class",
    "text": "Base geometry object. Before a geometry can be displayed, it must be converted to Model by the asset compiler (see man.AssetCompiler). To programmatically create a geometry use GeometryBuilder."
  },
  {
    "title": "GeometryBuilder",
    "href": "api/cpython/classes.html#geometrybuilder",
    "kind": "class",
    "text": "Use the geometry builder to programmatically create geometries. No optimization are performed by the geometry builder on the input data. To programmatically build a geometry for immediate display see ModelBuilder to directly build models."
  },
  {
    "title": "ImDrawList",
    "href": "api/cpython/classes.html#imdrawlist",
    "kind": "class",
    "text": "Immediate GUI drawing list. This object can be used to perform custom drawing operations on top of an imgui window."
  },
  {
    "title": "ImFont",
    "href": "api/cpython/classes.html#imfont",
    "kind": "class",
    "text": "Immediate GUI font."
  },
  {
    "title": "Instance",
    "href": "api/cpython/classes.html#instance",
    "kind": "class",
    "text": "Component to instantiate a scene as a child of a node upon setup."
  },
  {
    "title": "intList",
    "href": "api/cpython/classes.html#intlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "IntRect",
    "href": "api/cpython/classes.html#intrect",
    "kind": "class",
    "text": ""
  },
  {
    "title": "IsoSurface",
    "href": "api/cpython/classes.html#isosurface",
    "kind": "class",
    "text": "An iso-surface represents points of a constant value within a volume of space. This class holds a fixed-size 3-dimensional grid of values that can efficiently be converted to a Model at runtime."
  },
  {
    "title": "iVec2",
    "href": "api/cpython/classes.html#ivec2",
    "kind": "class",
    "text": "2-dimensional integer vector."
  },
  {
    "title": "iVec2List",
    "href": "api/cpython/classes.html#ivec2list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "IVideoStreamer",
    "href": "api/cpython/classes.html#ivideostreamer",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Joystick",
    "href": "api/cpython/classes.html#joystick",
    "kind": "class",
    "text": ""
  },
  {
    "title": "JoystickState",
    "href": "api/cpython/classes.html#joystickstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "JSON",
    "href": "api/cpython/classes.html#json",
    "kind": "class",
    "text": "JSON read/write object."
  },
  {
    "title": "Keyboard",
    "href": "api/cpython/classes.html#keyboard",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetKeyboardNames to query for available keyboard devices."
  },
  {
    "title": "KeyboardState",
    "href": "api/cpython/classes.html#keyboardstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Light",
    "href": "api/cpython/classes.html#light",
    "kind": "class",
    "text": "Add this component to a node to turn it into a light source, see man.ForwardPipeline."
  },
  {
    "title": "LuaObject",
    "href": "api/cpython/classes.html#luaobject",
    "kind": "class",
    "text": "Opaque reference to an Lua object. This type is used to transfer values between VMs, see man.Scripting."
  },
  {
    "title": "LuaObjectList",
    "href": "api/cpython/classes.html#luaobjectlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Mat3",
    "href": "api/cpython/classes.html#mat3",
    "kind": "class",
    "text": "A 3x3 matrix used to store rotation."
  },
  {
    "title": "Mat4",
    "href": "api/cpython/classes.html#mat4",
    "kind": "class",
    "text": "A 3x4 matrix used to store complete transformation including rotation, scale and position."
  },
  {
    "title": "Mat44",
    "href": "api/cpython/classes.html#mat44",
    "kind": "class",
    "text": "A 4x4 matrix used to store projection matrices."
  },
  {
    "title": "Mat4List",
    "href": "api/cpython/classes.html#mat4list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Material",
    "href": "api/cpython/classes.html#material",
    "kind": "class",
    "text": "High-level description of visual aspects of a surface. A material is comprised of a PipelineProgramRef, per-uniform value or texture, and a RenderState. See man.ForwardPipeline and man.PipelineShader."
  },
  {
    "title": "MaterialList",
    "href": "api/cpython/classes.html#materiallist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "MaterialRef",
    "href": "api/cpython/classes.html#materialref",
    "kind": "class",
    "text": "Reference to a Material inside a PipelineResources object."
  },
  {
    "title": "MinMax",
    "href": "api/cpython/classes.html#minmax",
    "kind": "class",
    "text": "3D bounding volume defined by a minimum and maximum position."
  },
  {
    "title": "Model",
    "href": "api/cpython/classes.html#model",
    "kind": "class",
    "text": "Runtime version of a Geometry. A model can be drawn to screen by calling DrawModel or by assigning it to the Object component of a node. To programmatically create a model see ModelBuilder."
  },
  {
    "title": "ModelBuilder",
    "href": "api/cpython/classes.html#modelbuilder",
    "kind": "class",
    "text": "Use the model builder to programmatically build models at runtime. The input data is optimized upon submission."
  },
  {
    "title": "ModelRef",
    "href": "api/cpython/classes.html#modelref",
    "kind": "class",
    "text": "Reference to a Model inside a PipelineResources object. See LoadModelFromFile, LoadModelFromAssets and PipelineResources_AddModel."
  },
  {
    "title": "Monitor",
    "href": "api/cpython/classes.html#monitor",
    "kind": "class",
    "text": ""
  },
  {
    "title": "MonitorList",
    "href": "api/cpython/classes.html#monitorlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "MonitorMode",
    "href": "api/cpython/classes.html#monitormode",
    "kind": "class",
    "text": ""
  },
  {
    "title": "MonitorModeList",
    "href": "api/cpython/classes.html#monitormodelist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Mouse",
    "href": "api/cpython/classes.html#mouse",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetMouseNames to query for available mouse devices."
  },
  {
    "title": "MouseState",
    "href": "api/cpython/classes.html#mousestate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "NavMesh",
    "href": "api/cpython/classes.html#navmesh",
    "kind": "class",
    "text": "Navigation mesh that can be queried using a NavMeshQuery to compute the most efficient path between two world-space positions."
  },
  {
    "title": "NavMeshQuery",
    "href": "api/cpython/classes.html#navmeshquery",
    "kind": "class",
    "text": "Navigation mesh query object. Queries are performed in world space. See man.Navigation."
  },
  {
    "title": "Node",
    "href": "api/cpython/classes.html#node",
    "kind": "class",
    "text": "The base element of a scene, see man.Scene."
  },
  {
    "title": "NodeList",
    "href": "api/cpython/classes.html#nodelist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "NodePairContacts",
    "href": "api/cpython/classes.html#nodepaircontacts",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Object",
    "href": "api/cpython/classes.html#object",
    "kind": "class",
    "text": "This components draws a Model. It stores the material table used to draw the model."
  },
  {
    "title": "OpenVREye",
    "href": "api/cpython/classes.html#openvreye",
    "kind": "class",
    "text": "Matrices for a VR eye, see OpenVRState."
  },
  {
    "title": "OpenVREyeFrameBuffer",
    "href": "api/cpython/classes.html#openvreyeframebuffer",
    "kind": "class",
    "text": "Framebuffer for a VR eye. Render to two such buffer, one for each eye, before submitting them using OpenVRSubmitFrame."
  },
  {
    "title": "OpenVRState",
    "href": "api/cpython/classes.html#openvrstate",
    "kind": "class",
    "text": "OpenVR state including the body and head transformations, the left and right eye states and the render target dimensions expected by the backend."
  },
  {
    "title": "OpenXREyeFrameBuffer",
    "href": "api/cpython/classes.html#openxreyeframebuffer",
    "kind": "class",
    "text": ""
  },
  {
    "title": "OpenXREyeFrameBufferList",
    "href": "api/cpython/classes.html#openxreyeframebufferlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "OpenXRFrameInfo",
    "href": "api/cpython/classes.html#openxrframeinfo",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Picture",
    "href": "api/cpython/classes.html#picture",
    "kind": "class",
    "text": "The picture origin (0, 0) is in the top-left corner of its frame with the X and Y axises increasing toward the right and bottom. To load and save a picture use LoadPicture, LoadPNG or SavePNG. The Picture_SetData and Picture_GetData methods"
  },
  {
    "title": "Pipeline",
    "href": "api/cpython/classes.html#pipeline",
    "kind": "class",
    "text": "Rendering pipeline base class."
  },
  {
    "title": "PipelineInfo",
    "href": "api/cpython/classes.html#pipelineinfo",
    "kind": "class",
    "text": ""
  },
  {
    "title": "PipelineProgram",
    "href": "api/cpython/classes.html#pipelineprogram",
    "kind": "class",
    "text": ""
  },
  {
    "title": "PipelineProgramRef",
    "href": "api/cpython/classes.html#pipelineprogramref",
    "kind": "class",
    "text": ""
  },
  {
    "title": "PipelineResources",
    "href": "api/cpython/classes.html#pipelineresources",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ProfilerFrame",
    "href": "api/cpython/classes.html#profilerframe",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ProgramHandle",
    "href": "api/cpython/classes.html#programhandle",
    "kind": "class",
    "text": "Handle to a shader program."
  },
  {
    "title": "Quaternion",
    "href": "api/cpython/classes.html#quaternion",
    "kind": "class",
    "text": "Quaternion can be used to represent a 3d rotation. It provides a more compact representation of the rotation than Mat3 and can efficiently and correctly interpolate (see Slerp) between two rotations."
  },
  {
    "title": "RaycastOut",
    "href": "api/cpython/classes.html#raycastout",
    "kind": "class",
    "text": "Contains the result of a physics raycast. * `P`: Position of the raycast hit * `N`: Normal of the raycast hit * `Node`: Node hit by the raycast * `t`: Parametric value of the intersection, ratio of the distance to the hit by the length of t"
  },
  {
    "title": "RaycastOutList",
    "href": "api/cpython/classes.html#raycastoutlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Rect",
    "href": "api/cpython/classes.html#rect",
    "kind": "class",
    "text": ""
  },
  {
    "title": "RenderState",
    "href": "api/cpython/classes.html#renderstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "RigidBody",
    "href": "api/cpython/classes.html#rigidbody",
    "kind": "class",
    "text": "Rigid body component, see man.Physics."
  },
  {
    "title": "SAO",
    "href": "api/cpython/classes.html#sao",
    "kind": "class",
    "text": "Ambient occlusion post-process object holding internal states and resources. Create with CreateSAOFromFile or CreateSAOFromAssets, use with ComputeSAO, finally call DestroySAO to dispose of resources when done."
  },
  {
    "title": "Scene",
    "href": "api/cpython/classes.html#scene",
    "kind": "class",
    "text": "A scene object representing a world populated with Node, see man.Scene."
  },
  {
    "title": "SceneAnimRef",
    "href": "api/cpython/classes.html#sceneanimref",
    "kind": "class",
    "text": "Reference to a scene animation."
  },
  {
    "title": "SceneAnimRefList",
    "href": "api/cpython/classes.html#sceneanimreflist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SceneBullet3Physics",
    "href": "api/cpython/classes.html#scenebullet3physics",
    "kind": "class",
    "text": "Newton physics for scene physics and collision components. See man.Physics."
  },
  {
    "title": "SceneBullet3PhysicsPreTickCallback",
    "href": "api/cpython/classes.html#scenebullet3physicspretickcallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SceneClocks",
    "href": "api/cpython/classes.html#sceneclocks",
    "kind": "class",
    "text": "Holds clocks for the different scene systems. This is required as some system such as the physics system may run at a different rate than the scene."
  },
  {
    "title": "SceneForwardPipelinePassViewId",
    "href": "api/cpython/classes.html#sceneforwardpipelinepassviewid",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SceneForwardPipelineRenderData",
    "href": "api/cpython/classes.html#sceneforwardpipelinerenderdata",
    "kind": "class",
    "text": "Holds all data required to draw a scene with the forward pipeline. See man.ForwardPipeline."
  },
  {
    "title": "SceneLuaVM",
    "href": "api/cpython/classes.html#sceneluavm",
    "kind": "class",
    "text": "Lua VM for scene script components. See man.Scripting."
  },
  {
    "title": "ScenePlayAnimRef",
    "href": "api/cpython/classes.html#sceneplayanimref",
    "kind": "class",
    "text": "Reference to a playing scene animation."
  },
  {
    "title": "ScenePlayAnimRefList",
    "href": "api/cpython/classes.html#sceneplayanimreflist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SceneView",
    "href": "api/cpython/classes.html#sceneview",
    "kind": "class",
    "text": "Holds a view to a subset of a scene. Used by the instance system to track instantiated scene content. See Node_GetInstanceSceneView and man.Scene."
  },
  {
    "title": "Script",
    "href": "api/cpython/classes.html#script",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ScriptList",
    "href": "api/cpython/classes.html#scriptlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SetDrawStatesCallback",
    "href": "api/cpython/classes.html#setdrawstatescallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Signal_returning_void_taking_const_char_ptr",
    "href": "api/cpython/classes.html#signal_returning_void_taking_const_char_ptr",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Signal_returning_void_taking_time_ns",
    "href": "api/cpython/classes.html#signal_returning_void_taking_time_ns",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SpatializedSourceState",
    "href": "api/cpython/classes.html#spatializedsourcestate",
    "kind": "class",
    "text": "State for a spatialized audio source, see man.Audio."
  },
  {
    "title": "SRanipalEyeState",
    "href": "api/cpython/classes.html#sranipaleyestate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SRanipalState",
    "href": "api/cpython/classes.html#sranipalstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "StereoSourceState",
    "href": "api/cpython/classes.html#stereosourcestate",
    "kind": "class",
    "text": "State for a stereo audio source, see man.Audio."
  },
  {
    "title": "StringList",
    "href": "api/cpython/classes.html#stringlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TextInputCallback",
    "href": "api/cpython/classes.html#textinputcallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TextInputCallbackConnection",
    "href": "api/cpython/classes.html#textinputcallbackconnection",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Texture",
    "href": "api/cpython/classes.html#texture",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TextureInfo",
    "href": "api/cpython/classes.html#textureinfo",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TextureRef",
    "href": "api/cpython/classes.html#textureref",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TimeCallback",
    "href": "api/cpython/classes.html#timecallback",
    "kind": "class",
    "text": "A function taking a time value as parameter with no return value, see man.CoordinateAndUnitSystem."
  },
  {
    "title": "TimeCallbackConnection",
    "href": "api/cpython/classes.html#timecallbackconnection",
    "kind": "class",
    "text": "A TimeCallback connection to a Signal_returning_void_taking_time_ns."
  },
  {
    "title": "Transform",
    "href": "api/cpython/classes.html#transform",
    "kind": "class",
    "text": "Transformation component for a Node, see man.Scene."
  },
  {
    "title": "TransformTRS",
    "href": "api/cpython/classes.html#transformtrs",
    "kind": "class",
    "text": "Translation, rotation and scale packed as a single object."
  },
  {
    "title": "UInt16List",
    "href": "api/cpython/classes.html#uint16list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "UInt32List",
    "href": "api/cpython/classes.html#uint32list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "UniformSetTexture",
    "href": "api/cpython/classes.html#uniformsettexture",
    "kind": "class",
    "text": "Command object to set a uniform texture at draw time."
  },
  {
    "title": "UniformSetTextureList",
    "href": "api/cpython/classes.html#uniformsettexturelist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "UniformSetValue",
    "href": "api/cpython/classes.html#uniformsetvalue",
    "kind": "class",
    "text": "Command object to set a uniform value at draw time."
  },
  {
    "title": "UniformSetValueList",
    "href": "api/cpython/classes.html#uniformsetvaluelist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "update_controllersCallback",
    "href": "api/cpython/classes.html#update_controllerscallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Vec2",
    "href": "api/cpython/classes.html#vec2",
    "kind": "class",
    "text": "2-dimensional floating point vector."
  },
  {
    "title": "Vec2List",
    "href": "api/cpython/classes.html#vec2list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Vec3",
    "href": "api/cpython/classes.html#vec3",
    "kind": "class",
    "text": "3-dimensional vector."
  },
  {
    "title": "Vec3List",
    "href": "api/cpython/classes.html#vec3list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Vec4",
    "href": "api/cpython/classes.html#vec4",
    "kind": "class",
    "text": "4-dimensional vector."
  },
  {
    "title": "Vec4List",
    "href": "api/cpython/classes.html#vec4list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Vertex",
    "href": "api/cpython/classes.html#vertex",
    "kind": "class",
    "text": ""
  },
  {
    "title": "VertexLayout",
    "href": "api/cpython/classes.html#vertexlayout",
    "kind": "class",
    "text": "Memory layout and types of vertex attributes."
  },
  {
    "title": "Vertices",
    "href": "api/cpython/classes.html#vertices",
    "kind": "class",
    "text": "Helper class to generate vertex buffers for drawing primitives."
  },
  {
    "title": "ViewState",
    "href": "api/cpython/classes.html#viewstate",
    "kind": "class",
    "text": "Everything required to define an observer inside a 3d world. This object holds the projection matrix and its associated frustum as well as the transformation of the observer. The world content is transformed by the observer view matrix befo"
  },
  {
    "title": "VoidPointer",
    "href": "api/cpython/classes.html#voidpointer",
    "kind": "class",
    "text": ""
  },
  {
    "title": "VRController",
    "href": "api/cpython/classes.html#vrcontroller",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetVRControllerNames to query for available VR controller devices."
  },
  {
    "title": "VRControllerState",
    "href": "api/cpython/classes.html#vrcontrollerstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "VRGenericTracker",
    "href": "api/cpython/classes.html#vrgenerictracker",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetVRGenericTrackerNames to query for available VR generic tracker devices."
  },
  {
    "title": "VRGenericTrackerState",
    "href": "api/cpython/classes.html#vrgenerictrackerstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Window",
    "href": "api/cpython/classes.html#window",
    "kind": "class",
    "text": "Window object."
  },
  {
    "title": "Abs",
    "href": "api/cpython/functions.html#abs",
    "kind": "function",
    "text": "Return the absolute value of the function input. For vectors, the absolute value is applied to each component individually and the resulting vector is returned."
  },
  {
    "title": "AddAssetsFolder",
    "href": "api/cpython/functions.html#addassetsfolder",
    "kind": "function",
    "text": "Mount a local filesystem folder as an assets source. See man.Assets."
  },
  {
    "title": "AddAssetsPackage",
    "href": "api/cpython/functions.html#addassetspackage",
    "kind": "function",
    "text": "Mount an archive stored on the local filesystem as an assets source. See man.Assets."
  },
  {
    "title": "AlphaScale",
    "href": "api/cpython/functions.html#alphascale",
    "kind": "function",
    "text": "Scale the alpha component of the input color."
  },
  {
    "title": "ApplyBloom",
    "href": "api/cpython/functions.html#applybloom",
    "kind": "function",
    "text": "Process `input` texture and generate a bloom overlay on top of `output`, input and output must be of the same size. Use CreateBloomFromFile/CreateBloomFromAssets to create a Bloom object and DestroyBloom to destroy its internal resources af"
  },
  {
    "title": "ARGB32",
    "href": "api/cpython/functions.html#argb32",
    "kind": "function",
    "text": "Create a 32 bit integer ARGB color."
  },
  {
    "title": "ARGB32ToRGBA32",
    "href": "api/cpython/functions.html#argb32torgba32",
    "kind": "function",
    "text": "Convert a 32 bit integer ARGB color to RGBA."
  },
  {
    "title": "AudioInit",
    "href": "api/cpython/functions.html#audioinit",
    "kind": "function",
    "text": "Initialize the audio system."
  },
  {
    "title": "AudioShutdown",
    "href": "api/cpython/functions.html#audioshutdown",
    "kind": "function",
    "text": "Shutdown the audio system."
  },
  {
    "title": "BaseToEuler",
    "href": "api/cpython/functions.html#basetoeuler",
    "kind": "function",
    "text": "Compute the Euler angles triplet for the provided `z` direction. The up-vector `y` can be provided to improve coherency of the returned values over time."
  },
  {
    "title": "BeginProfilerSection",
    "href": "api/cpython/functions.html#beginprofilersection",
    "kind": "function",
    "text": "Begin a named profiler section. Call EndProfilerSection to end the section."
  },
  {
    "title": "CaptureProfilerFrame",
    "href": "api/cpython/functions.html#captureprofilerframe",
    "kind": "function",
    "text": "Capture the current profiler frame but do not end it. See EndProfilerFrame to capture and end the current profiler frame. See PrintProfilerFrame to print a profiler frame to the console."
  },
  {
    "title": "CaptureTexture",
    "href": "api/cpython/functions.html#capturetexture",
    "kind": "function",
    "text": "Capture a texture content to a Picture. Return the frame counter at which the capture will be complete. A Picture object can be accessed by the CPU. This function is asynchronous and its result will not be available until the returned frame"
  },
  {
    "title": "Cast_Pipeline_To_ForwardPipeline",
    "href": "api/cpython/functions.html#cast_pipeline_to_forwardpipeline",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Ceil",
    "href": "api/cpython/functions.html#ceil",
    "kind": "function",
    "text": "Returns a vector whose elements are equal to the nearest integer greater than or equal to the vector elements."
  },
  {
    "title": "ChromaScale",
    "href": "api/cpython/functions.html#chromascale",
    "kind": "function",
    "text": "Return a copy of the color with its saturation scaled as specified."
  },
  {
    "title": "Clamp",
    "href": "api/cpython/functions.html#clamp",
    "kind": "function",
    "text": "Return a vector whose elements are equal to the vector elements clipped to the specified interval."
  },
  {
    "title": "ClampLen",
    "href": "api/cpython/functions.html#clamplen",
    "kind": "function",
    "text": "Returns a vector in the same direction as the specified vector, but with its length clipped by the specified interval."
  },
  {
    "title": "ClassifyLine",
    "href": "api/cpython/functions.html#classifyline",
    "kind": "function",
    "text": "Return `true` if the provided line intersect the bounding volume, `false` otherwise."
  },
  {
    "title": "ClassifySegment",
    "href": "api/cpython/functions.html#classifysegment",
    "kind": "function",
    "text": "Return `true` if the provided segment intersect the bounding volume, `false` otherwise."
  },
  {
    "title": "CleanPath",
    "href": "api/cpython/functions.html#cleanpath",
    "kind": "function",
    "text": "Cleanup a local filesystem path according to the host platform conventions. - Remove redundant folder separators. - Remove redundant `.` and `..` folder entries. - Ensure forward slash (`/`) folder separators on Unix and back slash (`\\`) fo"
  },
  {
    "title": "ClipSpaceToScreenSpace",
    "href": "api/cpython/functions.html#clipspacetoscreenspace",
    "kind": "function",
    "text": "Convert a 3d position in clip space (homogeneous space) to a 2d position on screen."
  },
  {
    "title": "Close",
    "href": "api/cpython/functions.html#close",
    "kind": "function",
    "text": "Close a file handle."
  },
  {
    "title": "Cm",
    "href": "api/cpython/functions.html#cm",
    "kind": "function",
    "text": "Convert a value in centimeters to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "ColorFromABGR32",
    "href": "api/cpython/functions.html#colorfromabgr32",
    "kind": "function",
    "text": "Create a color from a 32 bit ABGR integer."
  },
  {
    "title": "ColorFromRGBA32",
    "href": "api/cpython/functions.html#colorfromrgba32",
    "kind": "function",
    "text": "Create a color from a 32 bit RGBA integer."
  },
  {
    "title": "ColorFromVector3",
    "href": "api/cpython/functions.html#colorfromvector3",
    "kind": "function",
    "text": "Create a color from a 3d vector, alpha defaults to 1."
  },
  {
    "title": "ColorFromVector4",
    "href": "api/cpython/functions.html#colorfromvector4",
    "kind": "function",
    "text": "Return a 4-dimensional vector as a color."
  },
  {
    "title": "ColorI",
    "href": "api/cpython/functions.html#colori",
    "kind": "function",
    "text": "Create a color from integer values in the [0;255] range."
  },
  {
    "title": "ColorToABGR32",
    "href": "api/cpython/functions.html#colortoabgr32",
    "kind": "function",
    "text": "Return a 32 bit ABGR integer from a color."
  },
  {
    "title": "ColorToGrayscale",
    "href": "api/cpython/functions.html#colortograyscale",
    "kind": "function",
    "text": "Return the grayscale representation of a color. A weighted average is used to account for human perception of colors."
  },
  {
    "title": "ColorToRGBA32",
    "href": "api/cpython/functions.html#colortorgba32",
    "kind": "function",
    "text": "Return a 32 bit RGBA integer from a color."
  },
  {
    "title": "Compute2DProjectionMatrix",
    "href": "api/cpython/functions.html#compute2dprojectionmatrix",
    "kind": "function",
    "text": "Returns a projection matrix from a 2D space to the 3D world, as required by SetViewTransform for example."
  },
  {
    "title": "ComputeAspectRatioX",
    "href": "api/cpython/functions.html#computeaspectratiox",
    "kind": "function",
    "text": "Compute the aspect ratio factor for the provided viewport dimensions. Use this method to compute aspect ratio for landscape display. See ComputeAspectRatioY."
  },
  {
    "title": "ComputeAspectRatioY",
    "href": "api/cpython/functions.html#computeaspectratioy",
    "kind": "function",
    "text": "Compute the aspect ratio factor for the provided viewport dimensions. Use this method to compute aspect ratio for portrait display. See ComputeAspectRatioX."
  },
  {
    "title": "ComputeMinMaxBoundingSphere",
    "href": "api/cpython/functions.html#computeminmaxboundingsphere",
    "kind": "function",
    "text": "Compute the bounding sphere for the provided axis-aligned bounding box."
  },
  {
    "title": "ComputeOrthographicProjectionMatrix",
    "href": "api/cpython/functions.html#computeorthographicprojectionmatrix",
    "kind": "function",
    "text": "Compute an orthographic projection matrix. An orthographic projection has no perspective and all lines parrallel in 3d space will still appear parrallel on screen after projection using the returned matrix. The `size` parameter controls the"
  },
  {
    "title": "ComputeOrthographicViewState",
    "href": "api/cpython/functions.html#computeorthographicviewstate",
    "kind": "function",
    "text": "Compute an orthographic view state. The `size` parameter controls the extends of the projected view. When projecting a 3d world this parameter is expressed in meters. Use the `aspect_ratio` parameter to prevent distortion from induced by no"
  },
  {
    "title": "ComputePerspectiveProjectionMatrix",
    "href": "api/cpython/functions.html#computeperspectiveprojectionmatrix",
    "kind": "function",
    "text": "Compute a perspective projection matrix, , `fov` is the field of view angle, see Deg and Rad. See ZoomFactorToFov, FovToZoomFactor, ComputeAspectRatioX and ComputeAspectRatioY."
  },
  {
    "title": "ComputePerspectiveViewState",
    "href": "api/cpython/functions.html#computeperspectiveviewstate",
    "kind": "function",
    "text": "Compute a perspective view state. See ComputePerspectiveProjectionMatrix, ZoomFactorToFov, FovToZoomFactor, ComputeAspectRatioX and ComputeAspectRatioY."
  },
  {
    "title": "ComputeRenderState",
    "href": "api/cpython/functions.html#computerenderstate",
    "kind": "function",
    "text": "Compute a render state to control subsequent render calls culling mode, blending mode, Z mask, etc... The same render state can be used by different render calls. See DrawLines, DrawTriangles and DrawModel."
  },
  {
    "title": "ComputeSAO",
    "href": "api/cpython/functions.html#computesao",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ComputeSortKey",
    "href": "api/cpython/functions.html#computesortkey",
    "kind": "function",
    "text": "Compute a sorting key to control the rendering order of a display list, `view_depth` is expected in view space."
  },
  {
    "title": "ComputeSortKeyFromWorld",
    "href": "api/cpython/functions.html#computesortkeyfromworld",
    "kind": "function",
    "text": "Compute a sorting key to control the rendering order of a display list."
  },
  {
    "title": "ComputeTextHeight",
    "href": "api/cpython/functions.html#computetextheight",
    "kind": "function",
    "text": "Compute the height of a text string."
  },
  {
    "title": "ComputeTextRect",
    "href": "api/cpython/functions.html#computetextrect",
    "kind": "function",
    "text": "Compute the width and height of a text string."
  },
  {
    "title": "ConfigureCrowdAgent",
    "href": "api/cpython/functions.html#configurecrowdagent",
    "kind": "function",
    "text": "Create a parameter structure for a crowd agent to be used with a Crowd object."
  },
  {
    "title": "Contains",
    "href": "api/cpython/functions.html#contains",
    "kind": "function",
    "text": "Return `true` if the provided position is inside the bounding volume, `false` otherwise."
  },
  {
    "title": "CopyDir",
    "href": "api/cpython/functions.html#copydir",
    "kind": "function",
    "text": "Copy a directory on the local filesystem, this function does not recurse through subdirectories. See CopyDirRecursive."
  },
  {
    "title": "CopyDirRecursive",
    "href": "api/cpython/functions.html#copydirrecursive",
    "kind": "function",
    "text": "Copy a directory on the local filesystem, recurse through subdirectories."
  },
  {
    "title": "CopyFile",
    "href": "api/cpython/functions.html#copyfile",
    "kind": "function",
    "text": "Copy a file on the local filesystem."
  },
  {
    "title": "CosineInterpolate",
    "href": "api/cpython/functions.html#cosineinterpolate",
    "kind": "function",
    "text": "Compute the cosine interpolated value between `y0` and `y1` at `t`. See LinearInterpolate, CubicInterpolate and HermiteInterpolate."
  },
  {
    "title": "CreateBloomFromAssets",
    "href": "api/cpython/functions.html#createbloomfromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateBloomFromFile",
    "href": "api/cpython/functions.html#createbloomfromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateCamera",
    "href": "api/cpython/functions.html#createcamera",
    "kind": "function",
    "text": "Create a new Node with a Transform and Camera components."
  },
  {
    "title": "CreateCapsuleModel",
    "href": "api/cpython/functions.html#createcapsulemodel",
    "kind": "function",
    "text": "Create a capsule render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateConeModel",
    "href": "api/cpython/functions.html#createconemodel",
    "kind": "function",
    "text": "Create a cone render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateCubeModel",
    "href": "api/cpython/functions.html#createcubemodel",
    "kind": "function",
    "text": "Create a cube render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateCylinderModel",
    "href": "api/cpython/functions.html#createcylindermodel",
    "kind": "function",
    "text": "Create a cylinder render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateForwardPipeline",
    "href": "api/cpython/functions.html#createforwardpipeline",
    "kind": "function",
    "text": "Create a forward pipeline and its resources. See DestroyForwardPipeline."
  },
  {
    "title": "CreateForwardPipelineAAAFromAssets",
    "href": "api/cpython/functions.html#createforwardpipelineaaafromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateForwardPipelineAAAFromFile",
    "href": "api/cpython/functions.html#createforwardpipelineaaafromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateFrameBuffer",
    "href": "api/cpython/functions.html#createframebuffer",
    "kind": "function",
    "text": "Create a framebuffer and its texture attachments. See DestroyFrameBuffer."
  },
  {
    "title": "CreateInstanceFromAssets",
    "href": "api/cpython/functions.html#createinstancefromassets",
    "kind": "function",
    "text": "Helper function to create a Node with a Transform and an Instance component. The instance component will be setup and its resources loaded from the assets system. See man.Assets."
  },
  {
    "title": "CreateInstanceFromFile",
    "href": "api/cpython/functions.html#createinstancefromfile",
    "kind": "function",
    "text": "Helper function to create a Node with a Transform and an Instance component. The instance component will be setup and its resources loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "CreateLinearLight",
    "href": "api/cpython/functions.html#createlinearlight",
    "kind": "function",
    "text": "Helper function to create a Node with a Transform and a Light component."
  },
  {
    "title": "CreateMaterial",
    "href": "api/cpython/functions.html#creatematerial",
    "kind": "function",
    "text": "Helper function to create a material. See SetMaterialProgram, SetMaterialValue and SetMaterialTexture."
  },
  {
    "title": "CreateMissingMaterialProgramValuesFromAssets",
    "href": "api/cpython/functions.html#createmissingmaterialprogramvaluesfromassets",
    "kind": "function",
    "text": "This function scans the material program uniforms and creates a corresponding entry in the material if missing. Resources are loaded from the asset system if a default uniform value requires it. See man.Assets."
  },
  {
    "title": "CreateMissingMaterialProgramValuesFromFile",
    "href": "api/cpython/functions.html#createmissingmaterialprogramvaluesfromfile",
    "kind": "function",
    "text": "This function scans the material program uniforms and creates a corresponding entry in the material if missing. Resources are loaded from the local filesystem if a default uniform value requires it."
  },
  {
    "title": "CreateNavMeshQuery",
    "href": "api/cpython/functions.html#createnavmeshquery",
    "kind": "function",
    "text": "Create a navigation mesh query from a navigation mesh. See FindNavigationPathTo to perform an actual query."
  },
  {
    "title": "CreateObject",
    "href": "api/cpython/functions.html#createobject",
    "kind": "function",
    "text": "Create a Node with a Transform and Object components."
  },
  {
    "title": "CreateOrthographicCamera",
    "href": "api/cpython/functions.html#createorthographiccamera",
    "kind": "function",
    "text": "Create a Node with a Transform and a Camera component."
  },
  {
    "title": "CreatePhysicCube",
    "href": "api/cpython/functions.html#createphysiccube",
    "kind": "function",
    "text": "Create a Node with a Transform, Object and RigidBody components."
  },
  {
    "title": "CreatePhysicSphere",
    "href": "api/cpython/functions.html#createphysicsphere",
    "kind": "function",
    "text": "Create a Node with a Transform, Object and RigidBody components."
  },
  {
    "title": "CreatePlaneModel",
    "href": "api/cpython/functions.html#createplanemodel",
    "kind": "function",
    "text": "Create a plane render model."
  },
  {
    "title": "CreatePointLight",
    "href": "api/cpython/functions.html#createpointlight",
    "kind": "function",
    "text": "Create a Node with a Transform and a Light component."
  },
  {
    "title": "CreateSAOFromAssets",
    "href": "api/cpython/functions.html#createsaofromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateSAOFromFile",
    "href": "api/cpython/functions.html#createsaofromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateSceneRootNode",
    "href": "api/cpython/functions.html#createscenerootnode",
    "kind": "function",
    "text": "Helper function to create a Node with a Transform component then parent all root nodes in the scene to it."
  },
  {
    "title": "CreateScript",
    "href": "api/cpython/functions.html#createscript",
    "kind": "function",
    "text": "Helper function to create a Node with a Script component."
  },
  {
    "title": "CreateSphereModel",
    "href": "api/cpython/functions.html#createspheremodel",
    "kind": "function",
    "text": "Create a sphere render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateSpotLight",
    "href": "api/cpython/functions.html#createspotlight",
    "kind": "function",
    "text": "Create a Node with a Transform and a Light component."
  },
  {
    "title": "CreateTexture",
    "href": "api/cpython/functions.html#createtexture",
    "kind": "function",
    "text": "Create an empty texture. See CreateTextureFromPicture and UpdateTextureFromPicture."
  },
  {
    "title": "CreateTextureFromPicture",
    "href": "api/cpython/functions.html#createtexturefrompicture",
    "kind": "function",
    "text": "Create a texture from a picture. See Picture, CreateTexture and UpdateTextureFromPicture."
  },
  {
    "title": "Crop",
    "href": "api/cpython/functions.html#crop",
    "kind": "function",
    "text": "Crop a rectangle. Remove the specified amount of units on each side of the rectangle. See Grow."
  },
  {
    "title": "Cross",
    "href": "api/cpython/functions.html#cross",
    "kind": "function",
    "text": "Return the cross product of two vectors."
  },
  {
    "title": "CrossProductMat3",
    "href": "api/cpython/functions.html#crossproductmat3",
    "kind": "function",
    "text": "Creates a matrix __M__ so that __Mv = p⨯v__. Simply put, multiplying this matrix to any vector __v__ is equivalent to compute the cross product between __p__ and __v__."
  },
  {
    "title": "CubicInterpolate",
    "href": "api/cpython/functions.html#cubicinterpolate",
    "kind": "function",
    "text": "Perform a cubic interpolation across four values with `t` in the [0;1] range between `y1` and `y2`. See LinearInterpolate, CosineInterpolate and HermiteInterpolate."
  },
  {
    "title": "CutFileExtension",
    "href": "api/cpython/functions.html#cutfileextension",
    "kind": "function",
    "text": "Return a file path with its extension stripped. See CutFilePath and CutFileName."
  },
  {
    "title": "CutFileName",
    "href": "api/cpython/functions.html#cutfilename",
    "kind": "function",
    "text": "Return the name part of a file path. All folder navigation and extension are stripped. See CutFileExtension and CutFilePath."
  },
  {
    "title": "CutFilePath",
    "href": "api/cpython/functions.html#cutfilepath",
    "kind": "function",
    "text": "Return the folder navigation part of a file path. The file name and its extension are stripped. See CutFileExtension and CutFileName."
  },
  {
    "title": "Debug",
    "href": "api/cpython/functions.html#debug",
    "kind": "function",
    "text": ""
  },
  {
    "title": "DebugSceneExplorer",
    "href": "api/cpython/functions.html#debugsceneexplorer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Decompose",
    "href": "api/cpython/functions.html#decompose",
    "kind": "function",
    "text": "Decompose a transformation matrix into its translation, scaling and rotation components."
  },
  {
    "title": "Deg",
    "href": "api/cpython/functions.html#deg",
    "kind": "function",
    "text": "Convert an angle in degrees to the engine unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Deg3",
    "href": "api/cpython/functions.html#deg3",
    "kind": "function",
    "text": "Convert a triplet of angles in degrees to the engine unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "DegreeToRadian",
    "href": "api/cpython/functions.html#degreetoradian",
    "kind": "function",
    "text": "Convert an angle in degrees to radians."
  },
  {
    "title": "DestroyBloom",
    "href": "api/cpython/functions.html#destroybloom",
    "kind": "function",
    "text": "Destroy a bloom post process object and all associated resources."
  },
  {
    "title": "DestroyForwardPipeline",
    "href": "api/cpython/functions.html#destroyforwardpipeline",
    "kind": "function",
    "text": "Destroy a forward pipeline object."
  },
  {
    "title": "DestroyForwardPipelineAAA",
    "href": "api/cpython/functions.html#destroyforwardpipelineaaa",
    "kind": "function",
    "text": ""
  },
  {
    "title": "DestroyFrameBuffer",
    "href": "api/cpython/functions.html#destroyframebuffer",
    "kind": "function",
    "text": "Destroy a frame buffer and its resources."
  },
  {
    "title": "DestroyNavMesh",
    "href": "api/cpython/functions.html#destroynavmesh",
    "kind": "function",
    "text": "Destroy a navigation mesh object."
  },
  {
    "title": "DestroyNavMeshQuery",
    "href": "api/cpython/functions.html#destroynavmeshquery",
    "kind": "function",
    "text": "Destroy a navigation mesh query object."
  },
  {
    "title": "DestroyProgram",
    "href": "api/cpython/functions.html#destroyprogram",
    "kind": "function",
    "text": "Destroy a shader program."
  },
  {
    "title": "DestroySAO",
    "href": "api/cpython/functions.html#destroysao",
    "kind": "function",
    "text": "Destroy an ambient occlusion post process object and its resources."
  },
  {
    "title": "DestroyTexture",
    "href": "api/cpython/functions.html#destroytexture",
    "kind": "function",
    "text": "Destroy a texture object."
  },
  {
    "title": "DestroyWindow",
    "href": "api/cpython/functions.html#destroywindow",
    "kind": "function",
    "text": "Destroy a window object."
  },
  {
    "title": "Det",
    "href": "api/cpython/functions.html#det",
    "kind": "function",
    "text": "Return the determinant of a matrix."
  },
  {
    "title": "DisableCursor",
    "href": "api/cpython/functions.html#disablecursor",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Dist",
    "href": "api/cpython/functions.html#dist",
    "kind": "function",
    "text": "Return the Euclidean distance between two vectors."
  },
  {
    "title": "Dist2",
    "href": "api/cpython/functions.html#dist2",
    "kind": "function",
    "text": "Return the squared Euclidean distance between two vectors."
  },
  {
    "title": "DistanceToPlane",
    "href": "api/cpython/functions.html#distancetoplane",
    "kind": "function",
    "text": "Return the signed distance from point __p__ to a plane. - Distance is positive if __p__ is in front of the plane, meaning that the plane normal is pointing towards __p__. - Distance is negative if __p__ is behind the plane, meaning that the"
  },
  {
    "title": "Dot",
    "href": "api/cpython/functions.html#dot",
    "kind": "function",
    "text": "Return the dot product of two vectors."
  },
  {
    "title": "DrawLines",
    "href": "api/cpython/functions.html#drawlines",
    "kind": "function",
    "text": "Draw a list of lines to the specified view. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program."
  },
  {
    "title": "DrawModel",
    "href": "api/cpython/functions.html#drawmodel",
    "kind": "function",
    "text": "Draw a model to the specified view. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program."
  },
  {
    "title": "DrawNavMesh",
    "href": "api/cpython/functions.html#drawnavmesh",
    "kind": "function",
    "text": "Draw a navigation mesh to the specified view. This is function is for debugging purpose. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program."
  },
  {
    "title": "DrawSprites",
    "href": "api/cpython/functions.html#drawsprites",
    "kind": "function",
    "text": "Draw a list of sprites to the specified view. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program. *Note:* This function prepares the sprite on the CPU before submitting them all to the GPU as a si"
  },
  {
    "title": "DrawText",
    "href": "api/cpython/functions.html#drawtext",
    "kind": "function",
    "text": "Write text to the specified view using the provided shader program and uniform values."
  },
  {
    "title": "DrawTriangles",
    "href": "api/cpython/functions.html#drawtriangles",
    "kind": "function",
    "text": "Draw a list of triangles to the specified view. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program."
  },
  {
    "title": "DuplicateNodeAndChildrenFromAssets",
    "href": "api/cpython/functions.html#duplicatenodeandchildrenfromassets",
    "kind": "function",
    "text": "Duplicate a node and its child hierarchy. Resources will be loaded from the assets system. See man.Assets."
  },
  {
    "title": "DuplicateNodeAndChildrenFromFile",
    "href": "api/cpython/functions.html#duplicatenodeandchildrenfromfile",
    "kind": "function",
    "text": "Duplicate a node and its child hierarchy. Resources will be loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "DuplicateNodeFromAssets",
    "href": "api/cpython/functions.html#duplicatenodefromassets",
    "kind": "function",
    "text": "Duplicate a node. Resources will be loaded from the assets system. See man.Assets."
  },
  {
    "title": "DuplicateNodeFromFile",
    "href": "api/cpython/functions.html#duplicatenodefromfile",
    "kind": "function",
    "text": "Duplicate a node. Resources will be loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "DuplicateNodesAndChildrenFromAssets",
    "href": "api/cpython/functions.html#duplicatenodesandchildrenfromassets",
    "kind": "function",
    "text": "Duplicate each node and children hierarchy of a list. Resources will be loaded from the assets system. See man.Assets."
  },
  {
    "title": "DuplicateNodesAndChildrenFromFile",
    "href": "api/cpython/functions.html#duplicatenodesandchildrenfromfile",
    "kind": "function",
    "text": "Duplicate each node and children hierarchy of a list. Resources will be loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "DuplicateNodesFromAssets",
    "href": "api/cpython/functions.html#duplicatenodesfromassets",
    "kind": "function",
    "text": "Duplicate each node of a list. Resources will be loaded from the assets system."
  },
  {
    "title": "DuplicateNodesFromFile",
    "href": "api/cpython/functions.html#duplicatenodesfromfile",
    "kind": "function",
    "text": "Duplicate each node of a list. Resources will be loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "EndProfilerFrame",
    "href": "api/cpython/functions.html#endprofilerframe",
    "kind": "function",
    "text": "End a profiler frame and return it. See PrintProfilerFrame to print a profiler frame to the console."
  },
  {
    "title": "EndProfilerSection",
    "href": "api/cpython/functions.html#endprofilersection",
    "kind": "function",
    "text": "End a named profiler section. Call BeginProfilerSection to begin a new section."
  },
  {
    "title": "Error",
    "href": "api/cpython/functions.html#error",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Exists",
    "href": "api/cpython/functions.html#exists",
    "kind": "function",
    "text": "Return `true` if a file exists on the local filesystem, `false` otherwise."
  },
  {
    "title": "ExtractZoomFactorFromProjectionMatrix",
    "href": "api/cpython/functions.html#extractzoomfactorfromprojectionmatrix",
    "kind": "function",
    "text": "Extract zoom factor from a projection matrix. See ZoomFactorToFov."
  },
  {
    "title": "ExtractZRangeFromOrthographicProjectionMatrix",
    "href": "api/cpython/functions.html#extractzrangefromorthographicprojectionmatrix",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ExtractZRangeFromPerspectiveProjectionMatrix",
    "href": "api/cpython/functions.html#extractzrangefromperspectiveprojectionmatrix",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ExtractZRangeFromProjectionMatrix",
    "href": "api/cpython/functions.html#extractzrangefromprojectionmatrix",
    "kind": "function",
    "text": "Extract z near and z far clipping range from a projection matrix."
  },
  {
    "title": "FaceForward",
    "href": "api/cpython/functions.html#faceforward",
    "kind": "function",
    "text": "Return the provided vector facing toward the provided direction. If the angle between `v` and `d` is less than 90° then `v` is returned unchanged, `v` will be returned reversed otherwise."
  },
  {
    "title": "FactorizePath",
    "href": "api/cpython/functions.html#factorizepath",
    "kind": "function",
    "text": "Return the input path with all redundant navigation entries stripped (folder separator, `..` and `.` entries)."
  },
  {
    "title": "FileToString",
    "href": "api/cpython/functions.html#filetostring",
    "kind": "function",
    "text": "Return the content of a local filesystem as a string."
  },
  {
    "title": "FindNavigationPathTo",
    "href": "api/cpython/functions.html#findnavigationpathto",
    "kind": "function",
    "text": "Return the navigation path between `from` and `to` as a list of Vec3 world positions. See CreateNavMeshQuery."
  },
  {
    "title": "FitsInside",
    "href": "api/cpython/functions.html#fitsinside",
    "kind": "function",
    "text": "Return wether `a` fits in `b`."
  },
  {
    "title": "Floor",
    "href": "api/cpython/functions.html#floor",
    "kind": "function",
    "text": "Returns a vector whose elements are equal to the nearest integer less than or equal to the vector elements."
  },
  {
    "title": "FovToZoomFactor",
    "href": "api/cpython/functions.html#fovtozoomfactor",
    "kind": "function",
    "text": "Convert from a fov value in radian to a zoom factor value in meters."
  },
  {
    "title": "FpsController",
    "href": "api/cpython/functions.html#fpscontroller",
    "kind": "function",
    "text": "Implement a first-person-shooter like controller. The input position and rotation parameters are returned modified according to the state of the control keys. This function is usually used by passing the current camera position and rotation"
  },
  {
    "title": "Frame",
    "href": "api/cpython/functions.html#frame",
    "kind": "function",
    "text": "Advance the rendering backend to the next frame, execute all queued rendering commands. This function returns the backend current frame. The frame counter is used by asynchronous functions such as CaptureTexture. You must wait for the frame"
  },
  {
    "title": "FRand",
    "href": "api/cpython/functions.html#frand",
    "kind": "function",
    "text": "Return a random floating point value in the provided range, default range is [0;1]. See Rand to generate a random integer value."
  },
  {
    "title": "FromHLS",
    "href": "api/cpython/functions.html#fromhls",
    "kind": "function",
    "text": "Convert input hue/luminance/saturation color to RGBA, alpha channel is left unmodified."
  },
  {
    "title": "FRRand",
    "href": "api/cpython/functions.html#frrand",
    "kind": "function",
    "text": "Return a random floating point value in the provided range, default range is [-1;1]."
  },
  {
    "title": "GaussianBlurIsoSurface",
    "href": "api/cpython/functions.html#gaussianblurisosurface",
    "kind": "function",
    "text": "Apply a Gaussian blur to an iso-surface."
  },
  {
    "title": "GetArea",
    "href": "api/cpython/functions.html#getarea",
    "kind": "function",
    "text": "Return the area of the volume."
  },
  {
    "title": "GetCenter",
    "href": "api/cpython/functions.html#getcenter",
    "kind": "function",
    "text": "Return the center position of the volume."
  },
  {
    "title": "GetClock",
    "href": "api/cpython/functions.html#getclock",
    "kind": "function",
    "text": "Return the current clock since the last call to TickClock or ResetClock. See time_to_sec_f to convert the returned time to second."
  },
  {
    "title": "GetClockDt",
    "href": "api/cpython/functions.html#getclockdt",
    "kind": "function",
    "text": "Return the elapsed time recorded during the last call to TickClock."
  },
  {
    "title": "GetColorTexture",
    "href": "api/cpython/functions.html#getcolortexture",
    "kind": "function",
    "text": "Retrieves color texture attachment."
  },
  {
    "title": "GetColumn",
    "href": "api/cpython/functions.html#getcolumn",
    "kind": "function",
    "text": "Returns the nth column."
  },
  {
    "title": "GetCurrentWorkingDirectory",
    "href": "api/cpython/functions.html#getcurrentworkingdirectory",
    "kind": "function",
    "text": "Return the system current working directory."
  },
  {
    "title": "GetDepthTexture",
    "href": "api/cpython/functions.html#getdepthtexture",
    "kind": "function",
    "text": "Retrieves depth texture attachment."
  },
  {
    "title": "GetFileExtension",
    "href": "api/cpython/functions.html#getfileextension",
    "kind": "function",
    "text": "Return the extension part of a file path."
  },
  {
    "title": "GetFileName",
    "href": "api/cpython/functions.html#getfilename",
    "kind": "function",
    "text": "Return the name part of a file path (including its extension)."
  },
  {
    "title": "GetFilePath",
    "href": "api/cpython/functions.html#getfilepath",
    "kind": "function",
    "text": "Return the path part of a file path (excluding file name and extension)."
  },
  {
    "title": "GetForwardPipelineInfo",
    "href": "api/cpython/functions.html#getforwardpipelineinfo",
    "kind": "function",
    "text": "Return the pipeline info object for the forward pipeline."
  },
  {
    "title": "GetGamepadNames",
    "href": "api/cpython/functions.html#getgamepadnames",
    "kind": "function",
    "text": "Return a list of names for all supported gamepad devices on the system. See ReadGamepad."
  },
  {
    "title": "GetHandJointAngularVelocity",
    "href": "api/cpython/functions.html#gethandjointangularvelocity",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetHandJointLinearVelocity",
    "href": "api/cpython/functions.html#gethandjointlinearvelocity",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetHandJointPose",
    "href": "api/cpython/functions.html#gethandjointpose",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetHandJointRadius",
    "href": "api/cpython/functions.html#gethandjointradius",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetHeight",
    "href": "api/cpython/functions.html#getheight",
    "kind": "function",
    "text": "Return the height of a rectangle."
  },
  {
    "title": "GetJoystickDeviceNames",
    "href": "api/cpython/functions.html#getjoystickdevicenames",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetJoystickNames",
    "href": "api/cpython/functions.html#getjoysticknames",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetJsonBool",
    "href": "api/cpython/functions.html#getjsonbool",
    "kind": "function",
    "text": "Return the value of a boolean JSON key."
  },
  {
    "title": "GetJsonFloat",
    "href": "api/cpython/functions.html#getjsonfloat",
    "kind": "function",
    "text": "Return the value of a float JSON key."
  },
  {
    "title": "GetJsonInt",
    "href": "api/cpython/functions.html#getjsonint",
    "kind": "function",
    "text": "Return the value of an integer JSON key."
  },
  {
    "title": "GetJsonString",
    "href": "api/cpython/functions.html#getjsonstring",
    "kind": "function",
    "text": "Return the value of a string JSON key."
  },
  {
    "title": "GetKeyboardNames",
    "href": "api/cpython/functions.html#getkeyboardnames",
    "kind": "function",
    "text": "Return a list of names for all supported keyboard devices on the system. See ReadKeyboard."
  },
  {
    "title": "GetKeyName",
    "href": "api/cpython/functions.html#getkeyname",
    "kind": "function",
    "text": "Return the name for a keyboard key."
  },
  {
    "title": "GetMaterialAlphaCut",
    "href": "api/cpython/functions.html#getmaterialalphacut",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialAmbientUsesUV1",
    "href": "api/cpython/functions.html#getmaterialambientusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialBlendMode",
    "href": "api/cpython/functions.html#getmaterialblendmode",
    "kind": "function",
    "text": "Return a material blending mode."
  },
  {
    "title": "GetMaterialDepthTest",
    "href": "api/cpython/functions.html#getmaterialdepthtest",
    "kind": "function",
    "text": "Return a material depth test function."
  },
  {
    "title": "GetMaterialDiffuseUsesUV1",
    "href": "api/cpython/functions.html#getmaterialdiffuseusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialFaceCulling",
    "href": "api/cpython/functions.html#getmaterialfaceculling",
    "kind": "function",
    "text": "Return a material culling mode."
  },
  {
    "title": "GetMaterialNormalMapInWorldSpace",
    "href": "api/cpython/functions.html#getmaterialnormalmapinworldspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialSkinning",
    "href": "api/cpython/functions.html#getmaterialskinning",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialSpecularUsesUV1",
    "href": "api/cpython/functions.html#getmaterialspecularusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialTexture",
    "href": "api/cpython/functions.html#getmaterialtexture",
    "kind": "function",
    "text": "Return the texture reference assigned to a material named uniform."
  },
  {
    "title": "GetMaterialTextures",
    "href": "api/cpython/functions.html#getmaterialtextures",
    "kind": "function",
    "text": "Return the list of names of a material texture uniforms."
  },
  {
    "title": "GetMaterialValues",
    "href": "api/cpython/functions.html#getmaterialvalues",
    "kind": "function",
    "text": "Return the list of names of a material value uniforms."
  },
  {
    "title": "GetMaterialWriteRGBA",
    "href": "api/cpython/functions.html#getmaterialwritergba",
    "kind": "function",
    "text": "Return the material color mask."
  },
  {
    "title": "GetMaterialWriteZ",
    "href": "api/cpython/functions.html#getmaterialwritez",
    "kind": "function",
    "text": "Return the material depth write mask."
  },
  {
    "title": "GetMonitorModes",
    "href": "api/cpython/functions.html#getmonitormodes",
    "kind": "function",
    "text": "Return the list of supported monitor modes."
  },
  {
    "title": "GetMonitorName",
    "href": "api/cpython/functions.html#getmonitorname",
    "kind": "function",
    "text": "Return the monitor name."
  },
  {
    "title": "GetMonitorRect",
    "href": "api/cpython/functions.html#getmonitorrect",
    "kind": "function",
    "text": "Returns a rectangle going from the position, in screen coordinates, of the upper-left corner of the specified monitor to the position of the lower-right corner."
  },
  {
    "title": "GetMonitors",
    "href": "api/cpython/functions.html#getmonitors",
    "kind": "function",
    "text": "Return a list of monitors connected to the system."
  },
  {
    "title": "GetMonitorSizeMM",
    "href": "api/cpython/functions.html#getmonitorsizemm",
    "kind": "function",
    "text": "Returns the size, in millimetres, of the display area of the specified monitor."
  },
  {
    "title": "GetMouseNames",
    "href": "api/cpython/functions.html#getmousenames",
    "kind": "function",
    "text": "Return a list of names for all supported mouse devices on the system. See ReadKeyboard."
  },
  {
    "title": "GetNodePairContacts",
    "href": "api/cpython/functions.html#getnodepaircontacts",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetNodesInContact",
    "href": "api/cpython/functions.html#getnodesincontact",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetR",
    "href": "api/cpython/functions.html#getr",
    "kind": "function",
    "text": "See GetRotation."
  },
  {
    "title": "GetRMatrix",
    "href": "api/cpython/functions.html#getrmatrix",
    "kind": "function",
    "text": "See GetRotationMatrix."
  },
  {
    "title": "GetRotation",
    "href": "api/cpython/functions.html#getrotation",
    "kind": "function",
    "text": "Return the rotation component of a transformation matrix as a Euler triplet."
  },
  {
    "title": "GetRotationMatrix",
    "href": "api/cpython/functions.html#getrotationmatrix",
    "kind": "function",
    "text": "Return the rotation component of a transformation matrix as a Mat3 rotation matrix."
  },
  {
    "title": "GetRow",
    "href": "api/cpython/functions.html#getrow",
    "kind": "function",
    "text": "Returns the nth row of a matrix."
  },
  {
    "title": "GetS",
    "href": "api/cpython/functions.html#gets",
    "kind": "function",
    "text": "See GetScale."
  },
  {
    "title": "GetScale",
    "href": "api/cpython/functions.html#getscale",
    "kind": "function",
    "text": "Return the scale component of a matrix a scale vector."
  },
  {
    "title": "GetSceneForwardPipelineFog",
    "href": "api/cpython/functions.html#getsceneforwardpipelinefog",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetSceneForwardPipelineLights",
    "href": "api/cpython/functions.html#getsceneforwardpipelinelights",
    "kind": "function",
    "text": "Filter through the scene lights and return a list of pipeline lights to be used by the scene forward pipeline."
  },
  {
    "title": "GetSceneForwardPipelinePassViewId",
    "href": "api/cpython/functions.html#getsceneforwardpipelinepassviewid",
    "kind": "function",
    "text": "Return the view id for a scene forward pipeline pass id."
  },
  {
    "title": "GetSize",
    "href": "api/cpython/functions.html#getsize",
    "kind": "function",
    "text": "Return the size in bytes of a local file."
  },
  {
    "title": "GetSourceDuration",
    "href": "api/cpython/functions.html#getsourceduration",
    "kind": "function",
    "text": "Return the duration of an audio source."
  },
  {
    "title": "GetSourceState",
    "href": "api/cpython/functions.html#getsourcestate",
    "kind": "function",
    "text": "Return the state of an audio source."
  },
  {
    "title": "GetSourceTimecode",
    "href": "api/cpython/functions.html#getsourcetimecode",
    "kind": "function",
    "text": "Return the current timecode of a playing audio source."
  },
  {
    "title": "GetT",
    "href": "api/cpython/functions.html#gett",
    "kind": "function",
    "text": "See GetTranslation."
  },
  {
    "title": "GetTextures",
    "href": "api/cpython/functions.html#gettextures",
    "kind": "function",
    "text": "Returns color and depth texture attachments."
  },
  {
    "title": "GetTranslation",
    "href": "api/cpython/functions.html#gettranslation",
    "kind": "function",
    "text": "Return the translation part of a tranformation matrix as a translation vector."
  },
  {
    "title": "GetUserFolder",
    "href": "api/cpython/functions.html#getuserfolder",
    "kind": "function",
    "text": "Return the system user folder for the current user."
  },
  {
    "title": "GetVRControllerNames",
    "href": "api/cpython/functions.html#getvrcontrollernames",
    "kind": "function",
    "text": "Return a list of names for all supported VR controller devices on the system. See ReadVRController."
  },
  {
    "title": "GetVRGenericTrackerNames",
    "href": "api/cpython/functions.html#getvrgenerictrackernames",
    "kind": "function",
    "text": "Return a list of names for all supported VR tracker devices on the system."
  },
  {
    "title": "GetWidth",
    "href": "api/cpython/functions.html#getwidth",
    "kind": "function",
    "text": "Return the width of a rectangle."
  },
  {
    "title": "GetWindowClientSize",
    "href": "api/cpython/functions.html#getwindowclientsize",
    "kind": "function",
    "text": "Return a window client rectangle. The client area of a window does not include its decorations."
  },
  {
    "title": "GetWindowContentScale",
    "href": "api/cpython/functions.html#getwindowcontentscale",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetWindowHandle",
    "href": "api/cpython/functions.html#getwindowhandle",
    "kind": "function",
    "text": "Return the system native window handle."
  },
  {
    "title": "GetWindowInFocus",
    "href": "api/cpython/functions.html#getwindowinfocus",
    "kind": "function",
    "text": "Return the system window with input focus."
  },
  {
    "title": "GetWindowPos",
    "href": "api/cpython/functions.html#getwindowpos",
    "kind": "function",
    "text": "Return a window position on screen."
  },
  {
    "title": "GetWindowTitle",
    "href": "api/cpython/functions.html#getwindowtitle",
    "kind": "function",
    "text": "Return a window title."
  },
  {
    "title": "GetX",
    "href": "api/cpython/functions.html#getx",
    "kind": "function",
    "text": "Return the scaled X axis of a transformation matrix."
  },
  {
    "title": "GetY",
    "href": "api/cpython/functions.html#gety",
    "kind": "function",
    "text": "Return the scaled Y axis of a transformation matrix."
  },
  {
    "title": "GetZ",
    "href": "api/cpython/functions.html#getz",
    "kind": "function",
    "text": "Return the scaled Z axis of a transformation matrix."
  },
  {
    "title": "Grow",
    "href": "api/cpython/functions.html#grow",
    "kind": "function",
    "text": "Grow a rectangle by the specified amount of units. See Crop."
  },
  {
    "title": "HasFileExtension",
    "href": "api/cpython/functions.html#hasfileextension",
    "kind": "function",
    "text": "Test the extension of a file path."
  },
  {
    "title": "HermiteInterpolate",
    "href": "api/cpython/functions.html#hermiteinterpolate",
    "kind": "function",
    "text": "Perform a Hermite interpolation across four values with `t` in the [0;1] range between `y1` and `y2`. The `tension` and `bias` parameters can be used to control the shape of underlying interpolation curve. See LinearInterpolate, CosineInter"
  },
  {
    "title": "HideCursor",
    "href": "api/cpython/functions.html#hidecursor",
    "kind": "function",
    "text": "Hide the system mouse cursor. See ShowCursor."
  },
  {
    "title": "ImGuiAlignTextToFramePadding",
    "href": "api/cpython/functions.html#imguialigntexttoframepadding",
    "kind": "function",
    "text": "Vertically align upcoming text baseline to FramePadding __y__ coordinate so that it will align properly to regularly framed items."
  },
  {
    "title": "ImGuiBegin",
    "href": "api/cpython/functions.html#imguibegin",
    "kind": "function",
    "text": "Start a new window."
  },
  {
    "title": "ImGuiBeginChild",
    "href": "api/cpython/functions.html#imguibeginchild",
    "kind": "function",
    "text": "Begin a scrolling region."
  },
  {
    "title": "ImGuiBeginCombo",
    "href": "api/cpython/functions.html#imguibegincombo",
    "kind": "function",
    "text": "Begin a ImGui Combo Box."
  },
  {
    "title": "ImGuiBeginFrame",
    "href": "api/cpython/functions.html#imguibeginframe",
    "kind": "function",
    "text": "Begin an ImGui frame. This function must be called once per frame before any other ImGui call. When using multiple contexts, it must be called for each context you intend to use during the current frame. See ImGuiEndFrame."
  },
  {
    "title": "ImGuiBeginGroup",
    "href": "api/cpython/functions.html#imguibegingroup",
    "kind": "function",
    "text": "Lock horizontal starting position. Once closing a group it is seen as a single item (so you can use ImGuiIsItemHovered on a group, ImGuiSameLine between groups, etc...)."
  },
  {
    "title": "ImGuiBeginMainMenuBar",
    "href": "api/cpython/functions.html#imguibeginmainmenubar",
    "kind": "function",
    "text": "Create and append to a full screen menu-bar. Note: Only call ImGuiEndMainMenuBar if this returns `true`."
  },
  {
    "title": "ImGuiBeginMenu",
    "href": "api/cpython/functions.html#imguibeginmenu",
    "kind": "function",
    "text": "Create a sub-menu entry. Note: Only call ImGuiEndMenu if this returns `true`."
  },
  {
    "title": "ImGuiBeginMenuBar",
    "href": "api/cpython/functions.html#imguibeginmenubar",
    "kind": "function",
    "text": "Start append to the menu-bar of the current window (requires the `WindowFlags_MenuBar` flag). Note: Only call ImGuiEndMenuBar if this returns `true`."
  },
  {
    "title": "ImGuiBeginPopup",
    "href": "api/cpython/functions.html#imguibeginpopup",
    "kind": "function",
    "text": "Return `true` if popup is opened and starts outputting to it. Note: Only call ImGuiEndPopup if this returns `true`."
  },
  {
    "title": "ImGuiBeginPopupContextItem",
    "href": "api/cpython/functions.html#imguibeginpopupcontextitem",
    "kind": "function",
    "text": "ImGui helper to open and begin popup when clicked on last item."
  },
  {
    "title": "ImGuiBeginPopupContextVoid",
    "href": "api/cpython/functions.html#imguibeginpopupcontextvoid",
    "kind": "function",
    "text": "ImGui helper to open and begin popup when clicked in void (where there are no ImGui windows)"
  },
  {
    "title": "ImGuiBeginPopupContextWindow",
    "href": "api/cpython/functions.html#imguibeginpopupcontextwindow",
    "kind": "function",
    "text": "ImGui helper to open and begin popup when clicked on current window."
  },
  {
    "title": "ImGuiBeginPopupModal",
    "href": "api/cpython/functions.html#imguibeginpopupmodal",
    "kind": "function",
    "text": "Begin an ImGui modal dialog."
  },
  {
    "title": "ImGuiBeginTooltip",
    "href": "api/cpython/functions.html#imguibegintooltip",
    "kind": "function",
    "text": "Used to create full-featured tooltip windows that aren't just text."
  },
  {
    "title": "ImGuiBullet",
    "href": "api/cpython/functions.html#imguibullet",
    "kind": "function",
    "text": "Draw a small circle and keep the cursor on the same line. Advances by the same distance as an empty ImGuiTreeNode call."
  },
  {
    "title": "ImGuiBulletText",
    "href": "api/cpython/functions.html#imguibullettext",
    "kind": "function",
    "text": "Draw a bullet followed by a static text."
  },
  {
    "title": "ImGuiButton",
    "href": "api/cpython/functions.html#imguibutton",
    "kind": "function",
    "text": "Button widget returning `True` if the button was pressed."
  },
  {
    "title": "ImGuiCalcItemWidth",
    "href": "api/cpython/functions.html#imguicalcitemwidth",
    "kind": "function",
    "text": "Returns the width of item given pushed settings and current cursor position. Note: This is not necessarily the width of last item."
  },
  {
    "title": "ImGuiCalcTextSize",
    "href": "api/cpython/functions.html#imguicalctextsize",
    "kind": "function",
    "text": "Compute the bounding rectangle for the provided text."
  },
  {
    "title": "ImGuiCaptureKeyboardFromApp",
    "href": "api/cpython/functions.html#imguicapturekeyboardfromapp",
    "kind": "function",
    "text": "Force capture keyboard when your widget is being hovered."
  },
  {
    "title": "ImGuiCaptureMouseFromApp",
    "href": "api/cpython/functions.html#imguicapturemousefromapp",
    "kind": "function",
    "text": "Force capture mouse when your widget is being hovered."
  },
  {
    "title": "ImGuiCheckbox",
    "href": "api/cpython/functions.html#imguicheckbox",
    "kind": "function",
    "text": "Display a checkbox widget. Returns an interaction flag (user interacted with the widget) and the current widget state (checked or not after user interaction). ```python was_clicked, my_value = gs.ImGuiCheckBox('My value', my_value) ```"
  },
  {
    "title": "ImGuiClearInputBuffer",
    "href": "api/cpython/functions.html#imguiclearinputbuffer",
    "kind": "function",
    "text": "Force a reset of the ImGui input buffer."
  },
  {
    "title": "ImGuiCloseCurrentPopup",
    "href": "api/cpython/functions.html#imguiclosecurrentpopup",
    "kind": "function",
    "text": "Close the popup we have begin-ed into. Clicking on a menu item or selectable automatically closes the current popup."
  },
  {
    "title": "ImGuiCollapsingHeader",
    "href": "api/cpython/functions.html#imguicollapsingheader",
    "kind": "function",
    "text": "Draw a collapsing header, returns `False` if the header is collapsed so that you may skip drawing the header content."
  },
  {
    "title": "ImGuiColorButton",
    "href": "api/cpython/functions.html#imguicolorbutton",
    "kind": "function",
    "text": "Color button widget, display a small colored rectangle."
  },
  {
    "title": "ImGuiColorEdit",
    "href": "api/cpython/functions.html#imguicoloredit",
    "kind": "function",
    "text": "Color editor, returns the widget current color."
  },
  {
    "title": "ImGuiColumns",
    "href": "api/cpython/functions.html#imguicolumns",
    "kind": "function",
    "text": "Begin a column layout section. To move to the next column use ImGuiNextColumn. To end a column layout section pass `1` to this function. **Note:** Current implementation supports a maximum of 64 columns."
  },
  {
    "title": "ImGuiCombo",
    "href": "api/cpython/functions.html#imguicombo",
    "kind": "function",
    "text": "Combo box widget, return the current selection index. Combo items are passed as an array of string."
  },
  {
    "title": "ImGuiDragFloat",
    "href": "api/cpython/functions.html#imguidragfloat",
    "kind": "function",
    "text": "Declare a widget to edit a float value. The widget can be dragged over to modify the underlying value."
  },
  {
    "title": "ImGuiDragIntVec2",
    "href": "api/cpython/functions.html#imguidragintvec2",
    "kind": "function",
    "text": "Declare a widget to edit an iVec2 value. The widget can be dragged over to modify the underlying value."
  },
  {
    "title": "ImGuiDragVec2",
    "href": "api/cpython/functions.html#imguidragvec2",
    "kind": "function",
    "text": "Declare a float edit widget that can be dragged over to modify its value."
  },
  {
    "title": "ImGuiDragVec3",
    "href": "api/cpython/functions.html#imguidragvec3",
    "kind": "function",
    "text": "Declare a widget to edit a Vec3 value. The widget can be dragged over to modify the underlying value."
  },
  {
    "title": "ImGuiDragVec4",
    "href": "api/cpython/functions.html#imguidragvec4",
    "kind": "function",
    "text": "Declare a widget to edit a Vec4 value. The widget can be dragged over to modify the underlying value."
  },
  {
    "title": "ImGuiDummy",
    "href": "api/cpython/functions.html#imguidummy",
    "kind": "function",
    "text": "Add a dummy item of given size."
  },
  {
    "title": "ImGuiEnd",
    "href": "api/cpython/functions.html#imguiend",
    "kind": "function",
    "text": "End the current window."
  },
  {
    "title": "ImGuiEndChild",
    "href": "api/cpython/functions.html#imguiendchild",
    "kind": "function",
    "text": "End a scrolling region."
  },
  {
    "title": "ImGuiEndCombo",
    "href": "api/cpython/functions.html#imguiendcombo",
    "kind": "function",
    "text": "End a combo widget."
  },
  {
    "title": "ImGuiEndFrame",
    "href": "api/cpython/functions.html#imguiendframe",
    "kind": "function",
    "text": "End the current ImGui frame. All ImGui rendering is sent to the specified view. If no view is specified, view 255 is used. See man.Views."
  },
  {
    "title": "ImGuiEndGroup",
    "href": "api/cpython/functions.html#imguiendgroup",
    "kind": "function",
    "text": "End the current group."
  },
  {
    "title": "ImGuiEndMainMenuBar",
    "href": "api/cpython/functions.html#imguiendmainmenubar",
    "kind": "function",
    "text": "End the main menu bar. See ImGuiBeginMainMenuBar."
  },
  {
    "title": "ImGuiEndMenu",
    "href": "api/cpython/functions.html#imguiendmenu",
    "kind": "function",
    "text": "End the current sub-menu entry."
  },
  {
    "title": "ImGuiEndMenuBar",
    "href": "api/cpython/functions.html#imguiendmenubar",
    "kind": "function",
    "text": "End the current menu bar."
  },
  {
    "title": "ImGuiEndPopup",
    "href": "api/cpython/functions.html#imguiendpopup",
    "kind": "function",
    "text": "End the current popup."
  },
  {
    "title": "ImGuiEndTooltip",
    "href": "api/cpython/functions.html#imguiendtooltip",
    "kind": "function",
    "text": "End the current tooltip window. See ImGuiBeginTooltip."
  },
  {
    "title": "ImGuiGetColorU32",
    "href": "api/cpython/functions.html#imguigetcoloru32",
    "kind": "function",
    "text": "Return a style color component as a 32 bit unsigned integer. See ImGuiPushStyleColor."
  },
  {
    "title": "ImGuiGetColumnIndex",
    "href": "api/cpython/functions.html#imguigetcolumnindex",
    "kind": "function",
    "text": "Returns the index of the current column."
  },
  {
    "title": "ImGuiGetColumnOffset",
    "href": "api/cpython/functions.html#imguigetcolumnoffset",
    "kind": "function",
    "text": "Returns the current column offset in pixels, from the left side of the content region."
  },
  {
    "title": "ImGuiGetColumnsCount",
    "href": "api/cpython/functions.html#imguigetcolumnscount",
    "kind": "function",
    "text": "Return the number of columns in the current layout section. See ImGuiColumns."
  },
  {
    "title": "ImGuiGetColumnWidth",
    "href": "api/cpython/functions.html#imguigetcolumnwidth",
    "kind": "function",
    "text": "Returns the current column width in pixels."
  },
  {
    "title": "ImGuiGetContentRegionAvail",
    "href": "api/cpython/functions.html#imguigetcontentregionavail",
    "kind": "function",
    "text": "Get available space for content in the current layout."
  },
  {
    "title": "ImGuiGetContentRegionAvailWidth",
    "href": "api/cpython/functions.html#imguigetcontentregionavailwidth",
    "kind": "function",
    "text": "Helper function to return the available width of current content region. See ImGuiGetContentRegionAvail."
  },
  {
    "title": "ImGuiGetContentRegionMax",
    "href": "api/cpython/functions.html#imguigetcontentregionmax",
    "kind": "function",
    "text": "Return the available content space including window decorations and scrollbar."
  },
  {
    "title": "ImGuiGetCursorPos",
    "href": "api/cpython/functions.html#imguigetcursorpos",
    "kind": "function",
    "text": "Return the layout cursor position in window space. Next widget declaration will take place at the cursor position. See ImGuiSetCursorPos and ImGuiSameLine."
  },
  {
    "title": "ImGuiGetCursorPosX",
    "href": "api/cpython/functions.html#imguigetcursorposx",
    "kind": "function",
    "text": "Helper for ImGuiGetCursorPos."
  },
  {
    "title": "ImGuiGetCursorPosY",
    "href": "api/cpython/functions.html#imguigetcursorposy",
    "kind": "function",
    "text": "Helper for ImGuiGetCursorPos."
  },
  {
    "title": "ImGuiGetCursorScreenPos",
    "href": "api/cpython/functions.html#imguigetcursorscreenpos",
    "kind": "function",
    "text": "Return the current layout cursor position in screen space."
  },
  {
    "title": "ImGuiGetCursorStartPos",
    "href": "api/cpython/functions.html#imguigetcursorstartpos",
    "kind": "function",
    "text": "Return the current layout \"line\" starting position. See ImGuiSameLine."
  },
  {
    "title": "ImGuiGetFont",
    "href": "api/cpython/functions.html#imguigetfont",
    "kind": "function",
    "text": "Return the current ImGui font."
  },
  {
    "title": "ImGuiGetFontSize",
    "href": "api/cpython/functions.html#imguigetfontsize",
    "kind": "function",
    "text": "Return the font size (height in pixels) of the current ImGui font with the current scale applied."
  },
  {
    "title": "ImGuiGetFontTexUvWhitePixel",
    "href": "api/cpython/functions.html#imguigetfonttexuvwhitepixel",
    "kind": "function",
    "text": "Get UV coordinate for a while pixel, useful to draw custom shapes via the ImDrawList API."
  },
  {
    "title": "ImGuiGetFrameCount",
    "href": "api/cpython/functions.html#imguigetframecount",
    "kind": "function",
    "text": "Return the ImGui frame counter. See ImGuiBeginFrame and ImGuiEndFrame."
  },
  {
    "title": "ImGuiGetFrameHeightWithSpacing",
    "href": "api/cpython/functions.html#imguigetframeheightwithspacing",
    "kind": "function",
    "text": "Return the following value: FontSize + style.FramePadding.y * 2 + style.ItemSpacing.y (distance in pixels between 2 consecutive lines of framed widgets)"
  },
  {
    "title": "ImGuiGetID",
    "href": "api/cpython/functions.html#imguigetid",
    "kind": "function",
    "text": "Return a unique ImGui ID."
  },
  {
    "title": "ImGuiGetItemRectMax",
    "href": "api/cpython/functions.html#imguigetitemrectmax",
    "kind": "function",
    "text": "Get bounding rect maximum of last item in screen space."
  },
  {
    "title": "ImGuiGetItemRectMin",
    "href": "api/cpython/functions.html#imguigetitemrectmin",
    "kind": "function",
    "text": "Get bounding rect minimum of last item in screen space."
  },
  {
    "title": "ImGuiGetItemRectSize",
    "href": "api/cpython/functions.html#imguigetitemrectsize",
    "kind": "function",
    "text": "Get bounding rect size of last item in screen space."
  },
  {
    "title": "ImGuiGetMouseDragDelta",
    "href": "api/cpython/functions.html#imguigetmousedragdelta",
    "kind": "function",
    "text": "Return the distance covered by the mouse cursor since the last button press."
  },
  {
    "title": "ImGuiGetMousePos",
    "href": "api/cpython/functions.html#imguigetmousepos",
    "kind": "function",
    "text": "Return the mouse cursor coordinates in screen space."
  },
  {
    "title": "ImGuiGetMousePosOnOpeningCurrentPopup",
    "href": "api/cpython/functions.html#imguigetmouseposonopeningcurrentpopup",
    "kind": "function",
    "text": "Retrieve a backup of the mouse position at the time of opening the current popup. See ImGuiBeginPopup."
  },
  {
    "title": "ImGuiGetScrollMaxX",
    "href": "api/cpython/functions.html#imguigetscrollmaxx",
    "kind": "function",
    "text": "Get maximum scrolling amount on the horizontal axis."
  },
  {
    "title": "ImGuiGetScrollMaxY",
    "href": "api/cpython/functions.html#imguigetscrollmaxy",
    "kind": "function",
    "text": "Get maximum scrolling amount on the vertical axis."
  },
  {
    "title": "ImGuiGetScrollX",
    "href": "api/cpython/functions.html#imguigetscrollx",
    "kind": "function",
    "text": "Get scrolling amount on the horizontal axis."
  },
  {
    "title": "ImGuiGetScrollY",
    "href": "api/cpython/functions.html#imguigetscrolly",
    "kind": "function",
    "text": "Get scrolling amount on the vertical axis."
  },
  {
    "title": "ImGuiGetTextLineHeight",
    "href": "api/cpython/functions.html#imguigettextlineheight",
    "kind": "function",
    "text": "Return the height of a text line using the current font. See ImGuiPushFont."
  },
  {
    "title": "ImGuiGetTextLineHeightWithSpacing",
    "href": "api/cpython/functions.html#imguigettextlineheightwithspacing",
    "kind": "function",
    "text": "Return the height of a text line using the current font plus vertical spacing between two layout lines. See ImGuiGetTextLineHeight."
  },
  {
    "title": "ImGuiGetTime",
    "href": "api/cpython/functions.html#imguigettime",
    "kind": "function",
    "text": "Return the current ImGui time in seconds."
  },
  {
    "title": "ImGuiGetTreeNodeToLabelSpacing",
    "href": "api/cpython/functions.html#imguigettreenodetolabelspacing",
    "kind": "function",
    "text": "Return the horizontal distance preceding label when using ImGuiTreeNode or ImGuiBullet. The value `g.FontSize + style.FramePadding.x * 2` is returned for a regular unframed TreeNode."
  },
  {
    "title": "ImGuiGetWindowContentRegionMax",
    "href": "api/cpython/functions.html#imguigetwindowcontentregionmax",
    "kind": "function",
    "text": "Return the content boundaries max (roughly (0,0)+Size-Scroll) where Size can be override with ImGuiSetNextWindowContentSize, in window space."
  },
  {
    "title": "ImGuiGetWindowContentRegionMin",
    "href": "api/cpython/functions.html#imguigetwindowcontentregionmin",
    "kind": "function",
    "text": "Content boundaries min (roughly (0,0)-Scroll), in window space."
  },
  {
    "title": "ImGuiGetWindowContentRegionWidth",
    "href": "api/cpython/functions.html#imguigetwindowcontentregionwidth",
    "kind": "function",
    "text": "Return the width of the content region."
  },
  {
    "title": "ImGuiGetWindowDrawList",
    "href": "api/cpython/functions.html#imguigetwindowdrawlist",
    "kind": "function",
    "text": "Get the draw list associated to the current window, to append your own drawing primitives."
  },
  {
    "title": "ImGuiGetWindowHeight",
    "href": "api/cpython/functions.html#imguigetwindowheight",
    "kind": "function",
    "text": "Return the current window height."
  },
  {
    "title": "ImGuiGetWindowPos",
    "href": "api/cpython/functions.html#imguigetwindowpos",
    "kind": "function",
    "text": "Return the current window position in screen space. See ImGuiSetWindowPos."
  },
  {
    "title": "ImGuiGetWindowSize",
    "href": "api/cpython/functions.html#imguigetwindowsize",
    "kind": "function",
    "text": "Return the current window size. See ImGuiSetWindowSize."
  },
  {
    "title": "ImGuiGetWindowWidth",
    "href": "api/cpython/functions.html#imguigetwindowwidth",
    "kind": "function",
    "text": "Return the current window width."
  },
  {
    "title": "ImGuiImage",
    "href": "api/cpython/functions.html#imguiimage",
    "kind": "function",
    "text": "Display a texture as an image widget. See ImGuiImageButton."
  },
  {
    "title": "ImGuiImageButton",
    "href": "api/cpython/functions.html#imguiimagebutton",
    "kind": "function",
    "text": "Declare an image button displaying the provided texture. See ImGuiImage."
  },
  {
    "title": "ImGuiIndent",
    "href": "api/cpython/functions.html#imguiindent",
    "kind": "function",
    "text": "Move content position toward the right."
  },
  {
    "title": "ImGuiInit",
    "href": "api/cpython/functions.html#imguiinit",
    "kind": "function",
    "text": "Initialize the global ImGui context. This function must be called once before any other ImGui function using the global context. See ImGuiInitContext."
  },
  {
    "title": "ImGuiInitContext",
    "href": "api/cpython/functions.html#imguiinitcontext",
    "kind": "function",
    "text": "Initialize an ImGui context. This function must be called once before any other ImGui function using the context. See ImGuiInit."
  },
  {
    "title": "ImGuiInputFloat",
    "href": "api/cpython/functions.html#imguiinputfloat",
    "kind": "function",
    "text": "Float field widget."
  },
  {
    "title": "ImGuiInputInt",
    "href": "api/cpython/functions.html#imguiinputint",
    "kind": "function",
    "text": "Integer field widget."
  },
  {
    "title": "ImGuiInputIntVec2",
    "href": "api/cpython/functions.html#imguiinputintvec2",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiInputText",
    "href": "api/cpython/functions.html#imguiinputtext",
    "kind": "function",
    "text": "Text input widget, returns the current widget buffer content."
  },
  {
    "title": "ImGuiInputVec2",
    "href": "api/cpython/functions.html#imguiinputvec2",
    "kind": "function",
    "text": "Vec2 field widget."
  },
  {
    "title": "ImGuiInputVec3",
    "href": "api/cpython/functions.html#imguiinputvec3",
    "kind": "function",
    "text": "Vec3 field widget."
  },
  {
    "title": "ImGuiInputVec4",
    "href": "api/cpython/functions.html#imguiinputvec4",
    "kind": "function",
    "text": "Vec4 field widget."
  },
  {
    "title": "ImGuiInvisibleButton",
    "href": "api/cpython/functions.html#imguiinvisiblebutton",
    "kind": "function",
    "text": "Invisible button widget, return `True` if the button was pressed."
  },
  {
    "title": "ImGuiIsAnyItemActive",
    "href": "api/cpython/functions.html#imguiisanyitemactive",
    "kind": "function",
    "text": "Return `true` if any item is active, `false` otherwise."
  },
  {
    "title": "ImGuiIsAnyItemHovered",
    "href": "api/cpython/functions.html#imguiisanyitemhovered",
    "kind": "function",
    "text": "Return `true` if any item is hovered by the mouse cursor, `false` otherwise."
  },
  {
    "title": "ImGuiIsItemActive",
    "href": "api/cpython/functions.html#imguiisitemactive",
    "kind": "function",
    "text": "Was the last item active. e.g. button being held, text field being edited - items that do not interact will always return `false`."
  },
  {
    "title": "ImGuiIsItemClicked",
    "href": "api/cpython/functions.html#imguiisitemclicked",
    "kind": "function",
    "text": "Was the last item clicked."
  },
  {
    "title": "ImGuiIsItemHovered",
    "href": "api/cpython/functions.html#imguiisitemhovered",
    "kind": "function",
    "text": "Was the last item hovered by mouse."
  },
  {
    "title": "ImGuiIsItemVisible",
    "href": "api/cpython/functions.html#imguiisitemvisible",
    "kind": "function",
    "text": "Was the last item visible and not out of sight due to clipping/scrolling."
  },
  {
    "title": "ImGuiIsKeyDown",
    "href": "api/cpython/functions.html#imguiiskeydown",
    "kind": "function",
    "text": "Was the specified key down during the last frame?"
  },
  {
    "title": "ImGuiIsKeyPressed",
    "href": "api/cpython/functions.html#imguiiskeypressed",
    "kind": "function",
    "text": "Was the specified key pressed? A key press implies that the key was down and is currently released."
  },
  {
    "title": "ImGuiIsKeyReleased",
    "href": "api/cpython/functions.html#imguiiskeyreleased",
    "kind": "function",
    "text": "Was the specified key released during the last frame?"
  },
  {
    "title": "ImGuiIsMouseClicked",
    "href": "api/cpython/functions.html#imguiismouseclicked",
    "kind": "function",
    "text": "Was the specified mouse button clicked during the last frame? A mouse click implies that the button pressed earlier and released during the last frame."
  },
  {
    "title": "ImGuiIsMouseDoubleClicked",
    "href": "api/cpython/functions.html#imguiismousedoubleclicked",
    "kind": "function",
    "text": "Was the specified mouse button double-clicked during the last frame? A double-click implies two rapid successive clicks of the same button with the mouse cursor staying in the same position."
  },
  {
    "title": "ImGuiIsMouseDown",
    "href": "api/cpython/functions.html#imguiismousedown",
    "kind": "function",
    "text": "Was the specified mouse button down during the last frame?"
  },
  {
    "title": "ImGuiIsMouseDragging",
    "href": "api/cpython/functions.html#imguiismousedragging",
    "kind": "function",
    "text": "Is mouse dragging?"
  },
  {
    "title": "ImGuiIsMouseHoveringRect",
    "href": "api/cpython/functions.html#imguiismousehoveringrect",
    "kind": "function",
    "text": "Test whether the mouse cursor is hovering the specified rectangle."
  },
  {
    "title": "ImGuiIsMouseReleased",
    "href": "api/cpython/functions.html#imguiismousereleased",
    "kind": "function",
    "text": "Was the specified mouse button released during the last frame?"
  },
  {
    "title": "ImGuiIsRectVisible",
    "href": "api/cpython/functions.html#imguiisrectvisible",
    "kind": "function",
    "text": "Test if a rectangle of the specified size starting from cursor position is visible/not clipped. Or test if a rectangle in screen space is visible/not clipped."
  },
  {
    "title": "ImGuiIsWindowCollapsed",
    "href": "api/cpython/functions.html#imguiiswindowcollapsed",
    "kind": "function",
    "text": "Is the current window collapsed."
  },
  {
    "title": "ImGuiIsWindowFocused",
    "href": "api/cpython/functions.html#imguiiswindowfocused",
    "kind": "function",
    "text": "Is the current window focused."
  },
  {
    "title": "ImGuiIsWindowHovered",
    "href": "api/cpython/functions.html#imguiiswindowhovered",
    "kind": "function",
    "text": "Is the current window hovered and hoverable (not blocked by a popup), differentiates child windows from each others."
  },
  {
    "title": "ImGuiLabelText",
    "href": "api/cpython/functions.html#imguilabeltext",
    "kind": "function",
    "text": "Display text+label aligned the same way as value+label widgets."
  },
  {
    "title": "ImGuiListBox",
    "href": "api/cpython/functions.html#imguilistbox",
    "kind": "function",
    "text": "List widget."
  },
  {
    "title": "ImGuiMenuItem",
    "href": "api/cpython/functions.html#imguimenuitem",
    "kind": "function",
    "text": "Return `true` when activated. Shortcuts are displayed for convenience but not processed at the moment."
  },
  {
    "title": "ImGuiMouseDrawCursor",
    "href": "api/cpython/functions.html#imguimousedrawcursor",
    "kind": "function",
    "text": "Enable/disable the ImGui software mouse cursor."
  },
  {
    "title": "ImGuiNewFrame",
    "href": "api/cpython/functions.html#imguinewframe",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiNewLine",
    "href": "api/cpython/functions.html#imguinewline",
    "kind": "function",
    "text": "Undo a ImGuiSameLine call or force a new line when in an horizontal layout."
  },
  {
    "title": "ImGuiNextColumn",
    "href": "api/cpython/functions.html#imguinextcolumn",
    "kind": "function",
    "text": "Start the next column in multi-column layout. See ImGuiColumns."
  },
  {
    "title": "ImGuiOpenPopup",
    "href": "api/cpython/functions.html#imguiopenpopup",
    "kind": "function",
    "text": "Mark a named popup as open. Popup windows are closed when the user: * Clicks outside of their client rect, * Activates a pressable item, * ImGuiCloseCurrentPopup is called within a ImGuiBeginPopup/ImGuiEndPopup block. Popup identifiers are "
  },
  {
    "title": "ImGuiPopAllowKeyboardFocus",
    "href": "api/cpython/functions.html#imguipopallowkeyboardfocus",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushAllowKeyboardFocus."
  },
  {
    "title": "ImGuiPopButtonRepeat",
    "href": "api/cpython/functions.html#imguipopbuttonrepeat",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushButtonRepeat."
  },
  {
    "title": "ImGuiPopClipRect",
    "href": "api/cpython/functions.html#imguipopcliprect",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushClipRect."
  },
  {
    "title": "ImGuiPopFont",
    "href": "api/cpython/functions.html#imguipopfont",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushFont."
  },
  {
    "title": "ImGuiPopID",
    "href": "api/cpython/functions.html#imguipopid",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushID."
  },
  {
    "title": "ImGuiPopItemWidth",
    "href": "api/cpython/functions.html#imguipopitemwidth",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushItemWidth."
  },
  {
    "title": "ImGuiPopStyleColor",
    "href": "api/cpython/functions.html#imguipopstylecolor",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushStyleColor."
  },
  {
    "title": "ImGuiPopStyleVar",
    "href": "api/cpython/functions.html#imguipopstylevar",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushStyleVar."
  },
  {
    "title": "ImGuiPopTextWrapPos",
    "href": "api/cpython/functions.html#imguipoptextwrappos",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushTextWrapPos."
  },
  {
    "title": "ImGuiProgressBar",
    "href": "api/cpython/functions.html#imguiprogressbar",
    "kind": "function",
    "text": "Draw a progress bar, `fraction` must be between 0.0 and 1.0."
  },
  {
    "title": "ImGuiPushAllowKeyboardFocus",
    "href": "api/cpython/functions.html#imguipushallowkeyboardfocus",
    "kind": "function",
    "text": "Allow focusing using TAB/Shift-TAB, enabled by default but you can disable it for certain widgets."
  },
  {
    "title": "ImGuiPushButtonRepeat",
    "href": "api/cpython/functions.html#imguipushbuttonrepeat",
    "kind": "function",
    "text": "In repeat mode, `ButtonXXX` functions return repeated true in a typematic manner. Note that you can call ImGuiIsItemActive after any `Button` to tell if the button is held in the current frame."
  },
  {
    "title": "ImGuiPushClipRect",
    "href": "api/cpython/functions.html#imguipushcliprect",
    "kind": "function",
    "text": "Push a new clip rectangle onto the clipping stack."
  },
  {
    "title": "ImGuiPushFont",
    "href": "api/cpython/functions.html#imguipushfont",
    "kind": "function",
    "text": "Push a font on top of the font stack and make it current for subsequent text rendering operations."
  },
  {
    "title": "ImGuiPushID",
    "href": "api/cpython/functions.html#imguipushid",
    "kind": "function",
    "text": "Push a string into the ID stack."
  },
  {
    "title": "ImGuiPushItemWidth",
    "href": "api/cpython/functions.html#imguipushitemwidth",
    "kind": "function",
    "text": "Set the width of items for common large `item+label` widgets. - `>0`: width in pixels - `<0`: align `x` pixels to the right of window (so -1 always align width to the right side) - `=0`: default to ~2/3 of the window width See ImGuiPopItemW"
  },
  {
    "title": "ImGuiPushStyleColor",
    "href": "api/cpython/functions.html#imguipushstylecolor",
    "kind": "function",
    "text": "Push a value on the style stack for the specified style color. See ImGuiPopStyleColor."
  },
  {
    "title": "ImGuiPushStyleVar",
    "href": "api/cpython/functions.html#imguipushstylevar",
    "kind": "function",
    "text": "Push a value on the style stack for the specified style variable. See ImGuiPopStyleVar."
  },
  {
    "title": "ImGuiPushTextWrapPos",
    "href": "api/cpython/functions.html#imguipushtextwrappos",
    "kind": "function",
    "text": "Push word-wrapping position for text commands. - ` 0`: Wrap at `wrap_pos_x` position in window local space. See ImGuiPopTextWrapPos."
  },
  {
    "title": "ImGuiRadioButton",
    "href": "api/cpython/functions.html#imguiradiobutton",
    "kind": "function",
    "text": "Radio button widget, return the button state."
  },
  {
    "title": "ImGuiRender",
    "href": "api/cpython/functions.html#imguirender",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiResetMouseDragDelta",
    "href": "api/cpython/functions.html#imguiresetmousedragdelta",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiSameLine",
    "href": "api/cpython/functions.html#imguisameline",
    "kind": "function",
    "text": "Call between widgets or groups to layout them horizontally."
  },
  {
    "title": "ImGuiSelectable",
    "href": "api/cpython/functions.html#imguiselectable",
    "kind": "function",
    "text": "Selectable item. The following `width` values are possible: * `= 0.0`: Use remaining width. * `> 0.0`: Specific width. The following `height` values are possible: * `= 0.0`: Use label height. * `> 0.0`: Specific height."
  },
  {
    "title": "ImGuiSeparator",
    "href": "api/cpython/functions.html#imguiseparator",
    "kind": "function",
    "text": "Output an horizontal line to separate two distinct UI sections."
  },
  {
    "title": "ImGuiSetColumnOffset",
    "href": "api/cpython/functions.html#imguisetcolumnoffset",
    "kind": "function",
    "text": "Set the position of a column line in pixels, from the left side of the contents region."
  },
  {
    "title": "ImGuiSetColumnWidth",
    "href": "api/cpython/functions.html#imguisetcolumnwidth",
    "kind": "function",
    "text": "Set the column width in pixels."
  },
  {
    "title": "ImGuiSetCursorPos",
    "href": "api/cpython/functions.html#imguisetcursorpos",
    "kind": "function",
    "text": "Set the current widget output cursor position in window space."
  },
  {
    "title": "ImGuiSetCursorPosX",
    "href": "api/cpython/functions.html#imguisetcursorposx",
    "kind": "function",
    "text": "See ImGuiSetCursorPos."
  },
  {
    "title": "ImGuiSetCursorPosY",
    "href": "api/cpython/functions.html#imguisetcursorposy",
    "kind": "function",
    "text": "See ImGuiSetCursorPos."
  },
  {
    "title": "ImGuiSetCursorScreenPos",
    "href": "api/cpython/functions.html#imguisetcursorscreenpos",
    "kind": "function",
    "text": "Set the widget cursor output position in screen space."
  },
  {
    "title": "ImGuiSetItemAllowOverlap",
    "href": "api/cpython/functions.html#imguisetitemallowoverlap",
    "kind": "function",
    "text": "Allow the last item to be overlapped by a subsequent item. Sometimes useful with invisible buttons, selectables, etc... to catch unused areas."
  },
  {
    "title": "ImGuiSetItemDefaultFocus",
    "href": "api/cpython/functions.html#imguisetitemdefaultfocus",
    "kind": "function",
    "text": "Make the last item the default focused item of a window."
  },
  {
    "title": "ImGuiSetKeyboardFocusHere",
    "href": "api/cpython/functions.html#imguisetkeyboardfocushere",
    "kind": "function",
    "text": "Focus keyboard on the next widget. Use positive `offset` value to access sub components of a multiple component widget. Use `-1` to access the previous widget."
  },
  {
    "title": "ImGuiSetNextItemOpen",
    "href": "api/cpython/functions.html#imguisetnextitemopen",
    "kind": "function",
    "text": "Set next item open state."
  },
  {
    "title": "ImGuiSetNextWindowCollapsed",
    "href": "api/cpython/functions.html#imguisetnextwindowcollapsed",
    "kind": "function",
    "text": "Set next window collapsed state, call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowContentSize",
    "href": "api/cpython/functions.html#imguisetnextwindowcontentsize",
    "kind": "function",
    "text": "Set the size of the content area of the next declared window. Call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowContentWidth",
    "href": "api/cpython/functions.html#imguisetnextwindowcontentwidth",
    "kind": "function",
    "text": "See ImGuiSetNextWindowContentSize."
  },
  {
    "title": "ImGuiSetNextWindowFocus",
    "href": "api/cpython/functions.html#imguisetnextwindowfocus",
    "kind": "function",
    "text": "Set the next window to be focused/top-most. Call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowPos",
    "href": "api/cpython/functions.html#imguisetnextwindowpos",
    "kind": "function",
    "text": "Set next window position, call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowPosCenter",
    "href": "api/cpython/functions.html#imguisetnextwindowposcenter",
    "kind": "function",
    "text": "Set next window position to be centered on screen, call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowSize",
    "href": "api/cpython/functions.html#imguisetnextwindowsize",
    "kind": "function",
    "text": "Set next window size, call before ImGuiBegin. A value of 0 for an axis will auto-fit it."
  },
  {
    "title": "ImGuiSetNextWindowSizeConstraints",
    "href": "api/cpython/functions.html#imguisetnextwindowsizeconstraints",
    "kind": "function",
    "text": "Set the next window size limits. Use -1,-1 on either X/Y axis to preserve the current size. Sizes will be rounded down."
  },
  {
    "title": "ImGuiSetScrollFromPosY",
    "href": "api/cpython/functions.html#imguisetscrollfromposy",
    "kind": "function",
    "text": "Adjust scrolling amount to make a given position visible. Generally ImGuiGetCursorStartPos + offset to compute a valid position."
  },
  {
    "title": "ImGuiSetScrollHereY",
    "href": "api/cpython/functions.html#imguisetscrollherey",
    "kind": "function",
    "text": "Adjust scrolling amount to make current cursor position visible. - 0: Top. - 0.5: Center. - 1: Bottom. When using to make a default/current item visible, consider using ImGuiSetItemDefaultFocus instead."
  },
  {
    "title": "ImGuiSetScrollX",
    "href": "api/cpython/functions.html#imguisetscrollx",
    "kind": "function",
    "text": "Set scrolling amount between [0;ImGuiGetScrollMaxX]."
  },
  {
    "title": "ImGuiSetScrollY",
    "href": "api/cpython/functions.html#imguisetscrolly",
    "kind": "function",
    "text": "Set scrolling amount between [0;ImGuiGetScrollMaxY]."
  },
  {
    "title": "ImGuiSetTooltip",
    "href": "api/cpython/functions.html#imguisettooltip",
    "kind": "function",
    "text": "Set tooltip under mouse-cursor, typically used with ImGuiIsItemHovered/ImGuiIsAnyItemHovered. Last call wins."
  },
  {
    "title": "ImGuiSetWindowCollapsed",
    "href": "api/cpython/functions.html#imguisetwindowcollapsed",
    "kind": "function",
    "text": "Set named window collapsed state, prefer using ImGuiSetNextWindowCollapsed."
  },
  {
    "title": "ImGuiSetWindowFocus",
    "href": "api/cpython/functions.html#imguisetwindowfocus",
    "kind": "function",
    "text": "Set named window to be focused/top-most."
  },
  {
    "title": "ImGuiSetWindowFontScale",
    "href": "api/cpython/functions.html#imguisetwindowfontscale",
    "kind": "function",
    "text": "Per-window font scale."
  },
  {
    "title": "ImGuiSetWindowPos",
    "href": "api/cpython/functions.html#imguisetwindowpos",
    "kind": "function",
    "text": "Set named window position."
  },
  {
    "title": "ImGuiSetWindowSize",
    "href": "api/cpython/functions.html#imguisetwindowsize",
    "kind": "function",
    "text": "Set named window size."
  },
  {
    "title": "ImGuiShutdown",
    "href": "api/cpython/functions.html#imguishutdown",
    "kind": "function",
    "text": "Shutdown the global ImGui context."
  },
  {
    "title": "ImGuiSliderFloat",
    "href": "api/cpython/functions.html#imguisliderfloat",
    "kind": "function",
    "text": "Float slider widget."
  },
  {
    "title": "ImGuiSliderInt",
    "href": "api/cpython/functions.html#imguisliderint",
    "kind": "function",
    "text": "Integer slider widget."
  },
  {
    "title": "ImGuiSliderIntVec2",
    "href": "api/cpython/functions.html#imguisliderintvec2",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiSliderVec2",
    "href": "api/cpython/functions.html#imguislidervec2",
    "kind": "function",
    "text": "Vec2 slider widget."
  },
  {
    "title": "ImGuiSliderVec3",
    "href": "api/cpython/functions.html#imguislidervec3",
    "kind": "function",
    "text": "Vec3 slider widget."
  },
  {
    "title": "ImGuiSliderVec4",
    "href": "api/cpython/functions.html#imguislidervec4",
    "kind": "function",
    "text": "Vec4 slider widget."
  },
  {
    "title": "ImGuiSmallButton",
    "href": "api/cpython/functions.html#imguismallbutton",
    "kind": "function",
    "text": "Small button widget fitting the height of a text line, return `True` if the button was pressed."
  },
  {
    "title": "ImGuiSpacing",
    "href": "api/cpython/functions.html#imguispacing",
    "kind": "function",
    "text": "Add spacing."
  },
  {
    "title": "ImGuiText",
    "href": "api/cpython/functions.html#imguitext",
    "kind": "function",
    "text": "Static text."
  },
  {
    "title": "ImGuiTextColored",
    "href": "api/cpython/functions.html#imguitextcolored",
    "kind": "function",
    "text": "Colored static text."
  },
  {
    "title": "ImGuiTextDisabled",
    "href": "api/cpython/functions.html#imguitextdisabled",
    "kind": "function",
    "text": "Disabled static text."
  },
  {
    "title": "ImGuiTextUnformatted",
    "href": "api/cpython/functions.html#imguitextunformatted",
    "kind": "function",
    "text": "Raw text without formatting. Roughly equivalent to ImGuiText but faster, recommended for long chunks of text."
  },
  {
    "title": "ImGuiTextWrapped",
    "href": "api/cpython/functions.html#imguitextwrapped",
    "kind": "function",
    "text": "Wrapped static text. Note that this won't work on an auto-resizing window if there's no other widgets to extend the window width, you may need to set a size using ImGuiSetNextWindowSize."
  },
  {
    "title": "ImGuiTreeNode",
    "href": "api/cpython/functions.html#imguitreenode",
    "kind": "function",
    "text": "If returning `true` the node is open and the user is responsible for calling ImGuiTreePop."
  },
  {
    "title": "ImGuiTreeNodeEx",
    "href": "api/cpython/functions.html#imguitreenodeex",
    "kind": "function",
    "text": "See ImGuiTreeNode."
  },
  {
    "title": "ImGuiTreePop",
    "href": "api/cpython/functions.html#imguitreepop",
    "kind": "function",
    "text": "Pop the current tree node."
  },
  {
    "title": "ImGuiTreePush",
    "href": "api/cpython/functions.html#imguitreepush",
    "kind": "function",
    "text": "Already called by ImGuiTreeNode, but you can call ImGuiTreePush/ImGuiTreePop yourself for layouting purpose."
  },
  {
    "title": "ImGuiUnindent",
    "href": "api/cpython/functions.html#imguiunindent",
    "kind": "function",
    "text": "Move content position back to the left (cancel ImGuiIndent)."
  },
  {
    "title": "ImGuiWantCaptureMouse",
    "href": "api/cpython/functions.html#imguiwantcapturemouse",
    "kind": "function",
    "text": "ImGui wants mouse capture. Use this function to determine when to pause mouse processing from other parts of your program."
  },
  {
    "title": "Inch",
    "href": "api/cpython/functions.html#inch",
    "kind": "function",
    "text": "Convert a value in inches to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "InputInit",
    "href": "api/cpython/functions.html#inputinit",
    "kind": "function",
    "text": "Initialize the Input system. Must be invoked before any call to WindowSystemInit to work properly. ```python hg.InputInit() hg.WindowSystemInit() ```"
  },
  {
    "title": "InputShutdown",
    "href": "api/cpython/functions.html#inputshutdown",
    "kind": "function",
    "text": "Shutdown the Input system."
  },
  {
    "title": "Inside",
    "href": "api/cpython/functions.html#inside",
    "kind": "function",
    "text": "Test if a value is inside a containing volume."
  },
  {
    "title": "int_to_VoidPointer",
    "href": "api/cpython/functions.html#int_to_voidpointer",
    "kind": "function",
    "text": "Cast an integer to a void pointer. This function is only used to provide access to low-level structures and should not be needed most of the time."
  },
  {
    "title": "Intersection",
    "href": "api/cpython/functions.html#intersection",
    "kind": "function",
    "text": "Return the intersection of two rectangles."
  },
  {
    "title": "IntersectRay",
    "href": "api/cpython/functions.html#intersectray",
    "kind": "function",
    "text": "Intersect an infinite ray with an axis-aligned bounding box, if the first returned value is `true` it is followed by the near and far intersection points."
  },
  {
    "title": "Intersects",
    "href": "api/cpython/functions.html#intersects",
    "kind": "function",
    "text": "Return `true` if rect `a` intersects rect `b`."
  },
  {
    "title": "Inverse",
    "href": "api/cpython/functions.html#inverse",
    "kind": "function",
    "text": "Return the inverse of a matrix, vector or quaternion."
  },
  {
    "title": "InverseFast",
    "href": "api/cpython/functions.html#inversefast",
    "kind": "function",
    "text": "Compute the inverse of an orthonormal transformation matrix. This function is faster than the generic Inverse function but can only deal with a specific set of matrices. See Inverse."
  },
  {
    "title": "IsAssetFile",
    "href": "api/cpython/functions.html#isassetfile",
    "kind": "function",
    "text": "Test if an asset file exists in the assets system. See man.Assets."
  },
  {
    "title": "IsDir",
    "href": "api/cpython/functions.html#isdir",
    "kind": "function",
    "text": "Returns `true` if `path` is a directory on the local filesystem, `false` otherwise."
  },
  {
    "title": "IsEOF",
    "href": "api/cpython/functions.html#iseof",
    "kind": "function",
    "text": "Returns `true` if the cursor is at the end of the file, `false` otherwise."
  },
  {
    "title": "IsFile",
    "href": "api/cpython/functions.html#isfile",
    "kind": "function",
    "text": "Test if a file exists on the local filesystem."
  },
  {
    "title": "IsFinite",
    "href": "api/cpython/functions.html#isfinite",
    "kind": "function",
    "text": "Test if a floating point value is finite."
  },
  {
    "title": "IsHandJointActive",
    "href": "api/cpython/functions.html#ishandjointactive",
    "kind": "function",
    "text": ""
  },
  {
    "title": "IsMonitorConnected",
    "href": "api/cpython/functions.html#ismonitorconnected",
    "kind": "function",
    "text": "Test if the specified monitor is connected to the host device."
  },
  {
    "title": "IsoSurfaceSphere",
    "href": "api/cpython/functions.html#isosurfacesphere",
    "kind": "function",
    "text": "Output a sphere to an iso-surface."
  },
  {
    "title": "IsoSurfaceToModel",
    "href": "api/cpython/functions.html#isosurfacetomodel",
    "kind": "function",
    "text": "Convert an iso-surface to a render model, this function is geared toward efficiency and meant for realtime."
  },
  {
    "title": "IsPathAbsolute",
    "href": "api/cpython/functions.html#ispathabsolute",
    "kind": "function",
    "text": "Test if the provided path is an absolute or relative path."
  },
  {
    "title": "IsPrimaryMonitor",
    "href": "api/cpython/functions.html#isprimarymonitor",
    "kind": "function",
    "text": "Return `true` if the monitor is the primary host device monitor, `false` otherwise."
  },
  {
    "title": "IsValid",
    "href": "api/cpython/functions.html#isvalid",
    "kind": "function",
    "text": "Test if a resource if valid."
  },
  {
    "title": "IsWindowOpen",
    "href": "api/cpython/functions.html#iswindowopen",
    "kind": "function",
    "text": "Return `true` if the window is open, `false` otherwise."
  },
  {
    "title": "Km",
    "href": "api/cpython/functions.html#km",
    "kind": "function",
    "text": "Convert a value in kilometers to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Len",
    "href": "api/cpython/functions.html#len",
    "kind": "function",
    "text": "Return the length of the vector."
  },
  {
    "title": "Len2",
    "href": "api/cpython/functions.html#len2",
    "kind": "function",
    "text": "Return the length of the vector squared."
  },
  {
    "title": "Lerp",
    "href": "api/cpython/functions.html#lerp",
    "kind": "function",
    "text": "See LinearInterpolate."
  },
  {
    "title": "LerpAsOrthonormalBase",
    "href": "api/cpython/functions.html#lerpasorthonormalbase",
    "kind": "function",
    "text": "Linear interpolate between two transformation matrices on the [0;1] interval."
  },
  {
    "title": "LinearInterpolate",
    "href": "api/cpython/functions.html#linearinterpolate",
    "kind": "function",
    "text": "Linear interpolate between two values on the [0;1] interval. See CosineInterpolate, CubicInterpolate and HermiteInterpolate."
  },
  {
    "title": "ListDir",
    "href": "api/cpython/functions.html#listdir",
    "kind": "function",
    "text": "Get the content of a directory on the local filesystem, this function does not recurse into subfolders. See ListDirRecursive."
  },
  {
    "title": "ListDirRecursive",
    "href": "api/cpython/functions.html#listdirrecursive",
    "kind": "function",
    "text": "Get the content of a directory on the local filesystem, this function recurses into subfolders. See ListDir."
  },
  {
    "title": "LoadBMP",
    "href": "api/cpython/functions.html#loadbmp",
    "kind": "function",
    "text": "Load a Picture in [BMP](https://en.wikipedia.org/wiki/BMP_file_format) file format."
  },
  {
    "title": "LoadDataFromFile",
    "href": "api/cpython/functions.html#loaddatafromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadFontFromAssets",
    "href": "api/cpython/functions.html#loadfontfromassets",
    "kind": "function",
    "text": "Load a TrueType (TTF) font from the assets system. See man.Assets."
  },
  {
    "title": "LoadFontFromFile",
    "href": "api/cpython/functions.html#loadfontfromfile",
    "kind": "function",
    "text": "Load a TrueType (TTF) font from the local filesystem. See man.Assets."
  },
  {
    "title": "LoadForwardPipelineAAAConfigFromAssets",
    "href": "api/cpython/functions.html#loadforwardpipelineaaaconfigfromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadForwardPipelineAAAConfigFromFile",
    "href": "api/cpython/functions.html#loadforwardpipelineaaaconfigfromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadGIF",
    "href": "api/cpython/functions.html#loadgif",
    "kind": "function",
    "text": "Load a Picture in [GIF](https://en.wikipedia.org/wiki/GIF) file format."
  },
  {
    "title": "LoadJPG",
    "href": "api/cpython/functions.html#loadjpg",
    "kind": "function",
    "text": "Load a Picture in [JPEG](https://en.wikipedia.org/wiki/JPEG) file format."
  },
  {
    "title": "LoadJsonFromAssets",
    "href": "api/cpython/functions.html#loadjsonfromassets",
    "kind": "function",
    "text": "Load a JSON from the assets system. See man.Assets."
  },
  {
    "title": "LoadJsonFromFile",
    "href": "api/cpython/functions.html#loadjsonfromfile",
    "kind": "function",
    "text": "Load a JSON from the local filesystem."
  },
  {
    "title": "LoadModelFromAssets",
    "href": "api/cpython/functions.html#loadmodelfromassets",
    "kind": "function",
    "text": "Load a render model from the assets system. See DrawModel and man.Assets."
  },
  {
    "title": "LoadModelFromFile",
    "href": "api/cpython/functions.html#loadmodelfromfile",
    "kind": "function",
    "text": "Load a render model from the local filesystem."
  },
  {
    "title": "LoadNavMeshFromAssets",
    "href": "api/cpython/functions.html#loadnavmeshfromassets",
    "kind": "function",
    "text": "Load a navigation mesh from the assets system. See man.Assets."
  },
  {
    "title": "LoadNavMeshFromFile",
    "href": "api/cpython/functions.html#loadnavmeshfromfile",
    "kind": "function",
    "text": "Load a navigation mesh from the local filesystem."
  },
  {
    "title": "LoadOGGSoundAsset",
    "href": "api/cpython/functions.html#loadoggsoundasset",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadOGGSoundFile",
    "href": "api/cpython/functions.html#loadoggsoundfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadPicture",
    "href": "api/cpython/functions.html#loadpicture",
    "kind": "function",
    "text": "Load a Picture content from the filesystem."
  },
  {
    "title": "LoadPipelineProgramFromAssets",
    "href": "api/cpython/functions.html#loadpipelineprogramfromassets",
    "kind": "function",
    "text": "Load a pipeline shader program from the assets system. See man.Assets."
  },
  {
    "title": "LoadPipelineProgramFromFile",
    "href": "api/cpython/functions.html#loadpipelineprogramfromfile",
    "kind": "function",
    "text": "Load a pipeline shader program from the local filesystem."
  },
  {
    "title": "LoadPipelineProgramRefFromAssets",
    "href": "api/cpython/functions.html#loadpipelineprogramreffromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadPipelineProgramRefFromFile",
    "href": "api/cpython/functions.html#loadpipelineprogramreffromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadPNG",
    "href": "api/cpython/functions.html#loadpng",
    "kind": "function",
    "text": "Load a Picture in [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) file format."
  },
  {
    "title": "LoadProgramFromAssets",
    "href": "api/cpython/functions.html#loadprogramfromassets",
    "kind": "function",
    "text": "Load a shader program from the assets system. See man.Assets."
  },
  {
    "title": "LoadProgramFromFile",
    "href": "api/cpython/functions.html#loadprogramfromfile",
    "kind": "function",
    "text": "Load a shader program from the local filesystem."
  },
  {
    "title": "LoadPSD",
    "href": "api/cpython/functions.html#loadpsd",
    "kind": "function",
    "text": "Load a Picture in [PSD](https://en.wikipedia.org/wiki/Adobe_Photoshop#File_format) file format."
  },
  {
    "title": "LoadSceneBinaryFromAssets",
    "href": "api/cpython/functions.html#loadscenebinaryfromassets",
    "kind": "function",
    "text": "Load a scene in binary format from the assets system. Loaded content is added to the existing scene content. See man.Assets."
  },
  {
    "title": "LoadSceneBinaryFromDataAndAssets",
    "href": "api/cpython/functions.html#loadscenebinaryfromdataandassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadSceneBinaryFromDataAndFile",
    "href": "api/cpython/functions.html#loadscenebinaryfromdataandfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadSceneBinaryFromFile",
    "href": "api/cpython/functions.html#loadscenebinaryfromfile",
    "kind": "function",
    "text": "Load a scene in binary format from the local filesystem. Loaded content is added to the existing scene content."
  },
  {
    "title": "LoadSceneFromAssets",
    "href": "api/cpython/functions.html#loadscenefromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadSceneFromFile",
    "href": "api/cpython/functions.html#loadscenefromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadSceneJsonFromAssets",
    "href": "api/cpython/functions.html#loadscenejsonfromassets",
    "kind": "function",
    "text": "Load a scene in JSON format from the assets system. Loaded content is added to the existing scene content. See man.Assets."
  },
  {
    "title": "LoadSceneJsonFromFile",
    "href": "api/cpython/functions.html#loadscenejsonfromfile",
    "kind": "function",
    "text": "Load a scene in JSON format from the local filesystem. Loaded content is added to the existing scene content."
  },
  {
    "title": "LoadTextureFlagsFromAssets",
    "href": "api/cpython/functions.html#loadtextureflagsfromassets",
    "kind": "function",
    "text": "Load texture flags in the texture metafile from the assets system. See man.Assets."
  },
  {
    "title": "LoadTextureFlagsFromFile",
    "href": "api/cpython/functions.html#loadtextureflagsfromfile",
    "kind": "function",
    "text": "Load texture flags in the texture metafile from the local filesystem."
  },
  {
    "title": "LoadTextureFromAssets",
    "href": "api/cpython/functions.html#loadtexturefromassets",
    "kind": "function",
    "text": "Load a texture from the assets system. - When not using pipeline resources the texture informations are returned directly. - When using pipeline resources the texture informations can be retrieved from the PipelineResources object. See man."
  },
  {
    "title": "LoadTextureFromFile",
    "href": "api/cpython/functions.html#loadtexturefromfile",
    "kind": "function",
    "text": "Load a texture from the local filesystem. - When not using pipeline resources the texture informations are returned directly. - When using pipeline resources the texture informations can be retrieved from the PipelineResources object."
  },
  {
    "title": "LoadTGA",
    "href": "api/cpython/functions.html#loadtga",
    "kind": "function",
    "text": "Load a Picture in [TGA](https://en.wikipedia.org/wiki/Truevision_TGA) file format."
  },
  {
    "title": "LoadWAVSoundAsset",
    "href": "api/cpython/functions.html#loadwavsoundasset",
    "kind": "function",
    "text": "Load a sound in WAV format from the assets system and return a reference to it. See man.Assets."
  },
  {
    "title": "LoadWAVSoundFile",
    "href": "api/cpython/functions.html#loadwavsoundfile",
    "kind": "function",
    "text": "Load a sound in WAV format from the local filesystem and return a reference to it."
  },
  {
    "title": "Log",
    "href": "api/cpython/functions.html#log",
    "kind": "function",
    "text": ""
  },
  {
    "title": "MakeForwardPipelineLinearLight",
    "href": "api/cpython/functions.html#makeforwardpipelinelinearlight",
    "kind": "function",
    "text": "Create a forward pipeline linear light. See ForwardPipelineLights, PrepareForwardPipelineLights and SubmitModelToForwardPipeline."
  },
  {
    "title": "MakeForwardPipelinePointLight",
    "href": "api/cpython/functions.html#makeforwardpipelinepointlight",
    "kind": "function",
    "text": "Create a forward pipeline point light. See ForwardPipelineLights, PrepareForwardPipelineLights and SubmitModelToForwardPipeline."
  },
  {
    "title": "MakeForwardPipelineSpotLight",
    "href": "api/cpython/functions.html#makeforwardpipelinespotlight",
    "kind": "function",
    "text": "Create a forward pipeline spot light. See ForwardPipelineLights, PrepareForwardPipelineLights and SubmitModelToForwardPipeline."
  },
  {
    "title": "MakeFrustum",
    "href": "api/cpython/functions.html#makefrustum",
    "kind": "function",
    "text": "Create a projection frustum. This object can then be used to perform culling using TestVisibility. ```python # Compute a perspective matrix proj = hg.ComputePerspectiveProjectionMatrix(0.1, 1000, hg.FovToZoomFactor(math.pi/4), 1280/720) # M"
  },
  {
    "title": "MakePlane",
    "href": "api/cpython/functions.html#makeplane",
    "kind": "function",
    "text": "Geometrical plane in 3D space. - `p`: a point lying on the plane. - `n`: the plane normal. - `m`: an affine transformation matrix that will be applied to `p` and `n`."
  },
  {
    "title": "MakeRectFromWidthHeight",
    "href": "api/cpython/functions.html#makerectfromwidthheight",
    "kind": "function",
    "text": "Make a rectangle from width and height."
  },
  {
    "title": "MakeUniformSetTexture",
    "href": "api/cpython/functions.html#makeuniformsettexture",
    "kind": "function",
    "text": "Create a uniform set texture object. This object can be added to a UniformSetTextureList to control the shader program uniform values for a subsequent call to DrawModel."
  },
  {
    "title": "MakeUniformSetValue",
    "href": "api/cpython/functions.html#makeuniformsetvalue",
    "kind": "function",
    "text": "Create a uniform set value object. This object can be added to a UniformSetValueList to control the shader program uniform values for a subsequent call to DrawModel."
  },
  {
    "title": "MakeVec3",
    "href": "api/cpython/functions.html#makevec3",
    "kind": "function",
    "text": "Make a Vec3 from a Vec4. The input vector `w` component is discarded."
  },
  {
    "title": "MakeVertex",
    "href": "api/cpython/functions.html#makevertex",
    "kind": "function",
    "text": ""
  },
  {
    "title": "MakeVideoStreamer",
    "href": "api/cpython/functions.html#makevideostreamer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Mat3LookAt",
    "href": "api/cpython/functions.html#mat3lookat",
    "kind": "function",
    "text": "Return a rotation matrix looking down the provided vector. The input vector does not need to be normalized."
  },
  {
    "title": "Mat4LookAt",
    "href": "api/cpython/functions.html#mat4lookat",
    "kind": "function",
    "text": "Return a _look at_ matrix whose orientation points at the specified position."
  },
  {
    "title": "Mat4LookAtUp",
    "href": "api/cpython/functions.html#mat4lookatup",
    "kind": "function",
    "text": "Return a _look at_ matrix whose orientation points at the specified position and up direction."
  },
  {
    "title": "Mat4LookToward",
    "href": "api/cpython/functions.html#mat4looktoward",
    "kind": "function",
    "text": "Return a _look at_ matrix whose orientation points toward the specified direction."
  },
  {
    "title": "Mat4LookTowardUp",
    "href": "api/cpython/functions.html#mat4looktowardup",
    "kind": "function",
    "text": "Return a _look at_ matrix whose orientation points toward the specified directions."
  },
  {
    "title": "Max",
    "href": "api/cpython/functions.html#max",
    "kind": "function",
    "text": "Return a vector whose elements are the maximum of each of the two specified vectors."
  },
  {
    "title": "Min",
    "href": "api/cpython/functions.html#min",
    "kind": "function",
    "text": "Return a vector whose elements are the minimum of each of the two specified vectors."
  },
  {
    "title": "MinMaxFromPositionSize",
    "href": "api/cpython/functions.html#minmaxfrompositionsize",
    "kind": "function",
    "text": "Set `min = p - size/2` and `max = p + size/2`."
  },
  {
    "title": "MkDir",
    "href": "api/cpython/functions.html#mkdir",
    "kind": "function",
    "text": "Create a new directory. See MkTree."
  },
  {
    "title": "MkTree",
    "href": "api/cpython/functions.html#mktree",
    "kind": "function",
    "text": "Create a directory tree on the local filesystem. This function is recursive and creates each missing directory in the path. See MkDir."
  },
  {
    "title": "Mm",
    "href": "api/cpython/functions.html#mm",
    "kind": "function",
    "text": "Convert a value in millimeters to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Ms",
    "href": "api/cpython/functions.html#ms",
    "kind": "function",
    "text": "Convert a value in milliseconds to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Mtr",
    "href": "api/cpython/functions.html#mtr",
    "kind": "function",
    "text": "Convert a value in meters to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "NewFullscreenWindow",
    "href": "api/cpython/functions.html#newfullscreenwindow",
    "kind": "function",
    "text": "Create a new fullscreen window."
  },
  {
    "title": "NewIsoSurface",
    "href": "api/cpython/functions.html#newisosurface",
    "kind": "function",
    "text": "Return a new iso-surface object. See IsoSurfaceSphere to draw to an iso-surface and IsoSurfaceToModel to draw it."
  },
  {
    "title": "NewWindow",
    "href": "api/cpython/functions.html#newwindow",
    "kind": "function",
    "text": "Create a new window."
  },
  {
    "title": "NewWindowFrom",
    "href": "api/cpython/functions.html#newwindowfrom",
    "kind": "function",
    "text": "Wrap a native window handle in a Window object."
  },
  {
    "title": "Normalize",
    "href": "api/cpython/functions.html#normalize",
    "kind": "function",
    "text": "Return the input vector scaled so that its length is one."
  },
  {
    "title": "NormalizePath",
    "href": "api/cpython/functions.html#normalizepath",
    "kind": "function",
    "text": "Normalize a path according to the following conventions: - Replace all whitespaces by underscores."
  },
  {
    "title": "Offset",
    "href": "api/cpython/functions.html#offset",
    "kind": "function",
    "text": "Offset a rectangle by the specified amount of units."
  },
  {
    "title": "Open",
    "href": "api/cpython/functions.html#open",
    "kind": "function",
    "text": "Open a file in binary mode. See OpenText, OpenWrite, OpenWriteText"
  },
  {
    "title": "OpenFileDialog",
    "href": "api/cpython/functions.html#openfiledialog",
    "kind": "function",
    "text": "Open a native OpenFile dialog."
  },
  {
    "title": "OpenFolderDialog",
    "href": "api/cpython/functions.html#openfolderdialog",
    "kind": "function",
    "text": "Open a native OpenFolder dialog."
  },
  {
    "title": "OpenTemp",
    "href": "api/cpython/functions.html#opentemp",
    "kind": "function",
    "text": "Return a handle to a temporary file on the local filesystem."
  },
  {
    "title": "OpenText",
    "href": "api/cpython/functions.html#opentext",
    "kind": "function",
    "text": "Open a file as text. Return a handle to the opened file. See Open, OpenWrite, OpenWriteText"
  },
  {
    "title": "OpenVRCreateEyeFrameBuffer",
    "href": "api/cpython/functions.html#openvrcreateeyeframebuffer",
    "kind": "function",
    "text": "Creates and returns an man.VR eye framebuffer, with the desired level of anti-aliasing. This function must be invoked twice, for the left and right eyes."
  },
  {
    "title": "OpenVRDestroyEyeFrameBuffer",
    "href": "api/cpython/functions.html#openvrdestroyeyeframebuffer",
    "kind": "function",
    "text": "Destroy an eye framebuffer."
  },
  {
    "title": "OpenVRGetColorTexture",
    "href": "api/cpython/functions.html#openvrgetcolortexture",
    "kind": "function",
    "text": "Return the color texture attached to an eye framebuffer."
  },
  {
    "title": "OpenVRGetDepthTexture",
    "href": "api/cpython/functions.html#openvrgetdepthtexture",
    "kind": "function",
    "text": "Return the depth texture attached to an eye framebuffer."
  },
  {
    "title": "OpenVRGetFrameBufferSize",
    "href": "api/cpython/functions.html#openvrgetframebuffersize",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenVRGetState",
    "href": "api/cpython/functions.html#openvrgetstate",
    "kind": "function",
    "text": "Returns the current OpenVR state including the body, head and eye transformations."
  },
  {
    "title": "OpenVRInit",
    "href": "api/cpython/functions.html#openvrinit",
    "kind": "function",
    "text": "Initialize OpenVR. Start the device display, its controllers and trackers."
  },
  {
    "title": "OpenVRIsHMDMounted",
    "href": "api/cpython/functions.html#openvrishmdmounted",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenVRPostPresentHandoff",
    "href": "api/cpython/functions.html#openvrpostpresenthandoff",
    "kind": "function",
    "text": "Signal to the OpenVR compositor that it can immediatly start processing the current frame."
  },
  {
    "title": "OpenVRShutdown",
    "href": "api/cpython/functions.html#openvrshutdown",
    "kind": "function",
    "text": "Shutdown OpenVR."
  },
  {
    "title": "OpenVRStateToViewState",
    "href": "api/cpython/functions.html#openvrstatetoviewstate",
    "kind": "function",
    "text": "Compute the left and right eye view states from an OpenVR state. See OpenVRGetState."
  },
  {
    "title": "OpenVRSubmitFrame",
    "href": "api/cpython/functions.html#openvrsubmitframe",
    "kind": "function",
    "text": "Submit the left and right eye textures to the OpenVR compositor. See OpenVRCreateEyeFrameBuffer."
  },
  {
    "title": "OpenWrite",
    "href": "api/cpython/functions.html#openwrite",
    "kind": "function",
    "text": "Open a file as binary in write mode. See Open, OpenText, OpenWriteText"
  },
  {
    "title": "OpenWriteText",
    "href": "api/cpython/functions.html#openwritetext",
    "kind": "function",
    "text": "Open a file as text in write mode. See Open, OpenText, OpenWrite"
  },
  {
    "title": "OpenXRCreateEyeFrameBuffer",
    "href": "api/cpython/functions.html#openxrcreateeyeframebuffer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRDestroyEyeFrameBuffer",
    "href": "api/cpython/functions.html#openxrdestroyeyeframebuffer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRFinishSubmitFrameBuffer",
    "href": "api/cpython/functions.html#openxrfinishsubmitframebuffer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetColorTexture",
    "href": "api/cpython/functions.html#openxrgetcolortexture",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetColorTextureFromId",
    "href": "api/cpython/functions.html#openxrgetcolortexturefromid",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetDepthTexture",
    "href": "api/cpython/functions.html#openxrgetdepthtexture",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetDepthTextureFromId",
    "href": "api/cpython/functions.html#openxrgetdepthtexturefromid",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetEyeGaze",
    "href": "api/cpython/functions.html#openxrgeteyegaze",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetHeadPose",
    "href": "api/cpython/functions.html#openxrgetheadpose",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetInstanceInfo",
    "href": "api/cpython/functions.html#openxrgetinstanceinfo",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRInit",
    "href": "api/cpython/functions.html#openxrinit",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRShutdown",
    "href": "api/cpython/functions.html#openxrshutdown",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRSubmitSceneToForwardPipeline",
    "href": "api/cpython/functions.html#openxrsubmitscenetoforwardpipeline",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Orthonormalize",
    "href": "api/cpython/functions.html#orthonormalize",
    "kind": "function",
    "text": "Return a matrix where the row vectors form an orthonormal basis. All vectors are normalized and perpendicular to each other."
  },
  {
    "title": "Overlap",
    "href": "api/cpython/functions.html#overlap",
    "kind": "function",
    "text": "Return `true` if the provided volume overlaps with this volume, `false` otherwise. The test can optionally be restricted to a specific axis."
  },
  {
    "title": "PathJoin",
    "href": "api/cpython/functions.html#pathjoin",
    "kind": "function",
    "text": "Return a file path from a set of string elements."
  },
  {
    "title": "PathStartsWith",
    "href": "api/cpython/functions.html#pathstartswith",
    "kind": "function",
    "text": "Test if the provided path starts with the provided prefix."
  },
  {
    "title": "PathStripPrefix",
    "href": "api/cpython/functions.html#pathstripprefix",
    "kind": "function",
    "text": "Return a copy of the input path stripped of the provided prefix."
  },
  {
    "title": "PathStripSuffix",
    "href": "api/cpython/functions.html#pathstripsuffix",
    "kind": "function",
    "text": "Return a copy of the input path stripped of the provided suffix."
  },
  {
    "title": "PathToDisplay",
    "href": "api/cpython/functions.html#pathtodisplay",
    "kind": "function",
    "text": "Format a path for display."
  },
  {
    "title": "PauseSource",
    "href": "api/cpython/functions.html#pausesource",
    "kind": "function",
    "text": "Pause a playing audio source. See PlayStereo and PlaySpatialized."
  },
  {
    "title": "PlaySpatialized",
    "href": "api/cpython/functions.html#playspatialized",
    "kind": "function",
    "text": "Start playing a spatialized sound. Return a handle to the started source."
  },
  {
    "title": "PlayStereo",
    "href": "api/cpython/functions.html#playstereo",
    "kind": "function",
    "text": "Start playing a stereo sound. Return a handle to the started source."
  },
  {
    "title": "PrepareForwardPipelineLights",
    "href": "api/cpython/functions.html#prepareforwardpipelinelights",
    "kind": "function",
    "text": "Prepare a list of forward pipeline lights into a structure ready for submitting to the forward pipeline. Lights are sorted by priority/type and the most important lights are assigned to available lighting slot of the forward pipeline. See S"
  },
  {
    "title": "PrepareSceneForwardPipelineCommonRenderData",
    "href": "api/cpython/functions.html#preparesceneforwardpipelinecommonrenderdata",
    "kind": "function",
    "text": "Prepare the common render data to submit a scene to the forward pipeline. Note: When rendering multiple views of the same scene, common data only needs to be prepared once. See PrepareSceneForwardPipelineViewDependentRenderData."
  },
  {
    "title": "PrepareSceneForwardPipelineViewDependentRenderData",
    "href": "api/cpython/functions.html#preparesceneforwardpipelineviewdependentrenderdata",
    "kind": "function",
    "text": "Prepare the view dependent render data to submit a scene to the forward pipeline. See PrepareSceneForwardPipelineCommonRenderData."
  },
  {
    "title": "PrintProfilerFrame",
    "href": "api/cpython/functions.html#printprofilerframe",
    "kind": "function",
    "text": "Print a profiler frame to the console. Print all sections in the frame, their duration and event count."
  },
  {
    "title": "ProcessLoadQueues",
    "href": "api/cpython/functions.html#processloadqueues",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ProcessModelLoadQueue",
    "href": "api/cpython/functions.html#processmodelloadqueue",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ProcessTextureLoadQueue",
    "href": "api/cpython/functions.html#processtextureloadqueue",
    "kind": "function",
    "text": "Process the texture load queue. This function must be called to load textures queued while loading a scene or model with the LSSF_QueueTextureLoads flag. See LoadSaveSceneFlags."
  },
  {
    "title": "ProjectOrthoToClipSpace",
    "href": "api/cpython/functions.html#projectorthotoclipspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ProjectOrthoToScreenSpace",
    "href": "api/cpython/functions.html#projectorthotoscreenspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ProjectToClipSpace",
    "href": "api/cpython/functions.html#projecttoclipspace",
    "kind": "function",
    "text": "Project a world position to the clipping space."
  },
  {
    "title": "ProjectToScreenSpace",
    "href": "api/cpython/functions.html#projecttoscreenspace",
    "kind": "function",
    "text": "Project a world position to screen coordinates."
  },
  {
    "title": "ProjectZToClipSpace",
    "href": "api/cpython/functions.html#projectztoclipspace",
    "kind": "function",
    "text": "Project a depth value to clip space."
  },
  {
    "title": "Quantize",
    "href": "api/cpython/functions.html#quantize",
    "kind": "function",
    "text": "Return the provided value quantized to the specified step."
  },
  {
    "title": "QuaternionFromAxisAngle",
    "href": "api/cpython/functions.html#quaternionfromaxisangle",
    "kind": "function",
    "text": "Return a quaternion rotation from a 3d axis and a rotation around that axis."
  },
  {
    "title": "QuaternionFromEuler",
    "href": "api/cpython/functions.html#quaternionfromeuler",
    "kind": "function",
    "text": "Return a quaternion 3d rotation from its _Euler_ vector representation."
  },
  {
    "title": "QuaternionFromMatrix3",
    "href": "api/cpython/functions.html#quaternionfrommatrix3",
    "kind": "function",
    "text": "Return a quaternion rotation from its Mat3 representation."
  },
  {
    "title": "QuaternionLookAt",
    "href": "api/cpython/functions.html#quaternionlookat",
    "kind": "function",
    "text": "Return a quaternion 3d rotation oriented toward the specified position when sitting on the world's origin _{0, 0, 0}_."
  },
  {
    "title": "Rad",
    "href": "api/cpython/functions.html#rad",
    "kind": "function",
    "text": "Convert an angle in radians to the engine unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Rad3",
    "href": "api/cpython/functions.html#rad3",
    "kind": "function",
    "text": "Convert a triplet of angles in radians to the engine unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "RadianToDegree",
    "href": "api/cpython/functions.html#radiantodegree",
    "kind": "function",
    "text": "Convert an angle in radians to degrees."
  },
  {
    "title": "Rand",
    "href": "api/cpython/functions.html#rand",
    "kind": "function",
    "text": "Return a random integer value in the provided range, default range is [0;65535]. See FRand to generate a random floating point value."
  },
  {
    "title": "RandomVec3",
    "href": "api/cpython/functions.html#randomvec3",
    "kind": "function",
    "text": "Return a vector with each component randomized in the inclusive provided range."
  },
  {
    "title": "RandomVec4",
    "href": "api/cpython/functions.html#randomvec4",
    "kind": "function",
    "text": "Return a vector with each component randomized in the inclusive provided range."
  },
  {
    "title": "ReadFloat",
    "href": "api/cpython/functions.html#readfloat",
    "kind": "function",
    "text": "Read a binary 32 bit floating point value from a local file."
  },
  {
    "title": "ReadGamepad",
    "href": "api/cpython/functions.html#readgamepad",
    "kind": "function",
    "text": "Read the current state of a named gamepad. If no name is passed, `default` is implied. See GetGamepadNames."
  },
  {
    "title": "ReadJoystick",
    "href": "api/cpython/functions.html#readjoystick",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ReadKeyboard",
    "href": "api/cpython/functions.html#readkeyboard",
    "kind": "function",
    "text": "Read the current state of a named keyboard. If no name is passed, `default` is implied. See GetKeyboardNames."
  },
  {
    "title": "ReadMouse",
    "href": "api/cpython/functions.html#readmouse",
    "kind": "function",
    "text": "Read the current state of a named mouse. If no name is passed, `default` is implied. See GetMouseNames."
  },
  {
    "title": "ReadString",
    "href": "api/cpython/functions.html#readstring",
    "kind": "function",
    "text": "Read a binary string from a local file. Strings are stored as a `uint32_t length` field followed by the string content in UTF-8."
  },
  {
    "title": "ReadUInt16",
    "href": "api/cpython/functions.html#readuint16",
    "kind": "function",
    "text": "Read a binary 16 bit unsigned integer value from a local file."
  },
  {
    "title": "ReadUInt32",
    "href": "api/cpython/functions.html#readuint32",
    "kind": "function",
    "text": "Read a binary 32 bit unsigned integer value from a local file."
  },
  {
    "title": "ReadUInt8",
    "href": "api/cpython/functions.html#readuint8",
    "kind": "function",
    "text": "Read a binary 8 bit unsigned integer value from a local file."
  },
  {
    "title": "ReadVRController",
    "href": "api/cpython/functions.html#readvrcontroller",
    "kind": "function",
    "text": "Read the current state of a named VR controller. If no name is passed, `default` is implied. See GetVRControllerNames."
  },
  {
    "title": "ReadVRGenericTracker",
    "href": "api/cpython/functions.html#readvrgenerictracker",
    "kind": "function",
    "text": "Read the current state of a named VR generic tracked. If no name is passed, `default` is implied. See GetVRGenericTrackerNames."
  },
  {
    "title": "Reflect",
    "href": "api/cpython/functions.html#reflect",
    "kind": "function",
    "text": "Return the input vector reflected around the specified normal."
  },
  {
    "title": "Refract",
    "href": "api/cpython/functions.html#refract",
    "kind": "function",
    "text": "Return the input vector refracted around the provided surface normal. - `k_in`: IOR of the medium the vector is exiting. - `k_out`: IOR of the medium the vector is entering."
  },
  {
    "title": "RemoveAssetsFolder",
    "href": "api/cpython/functions.html#removeassetsfolder",
    "kind": "function",
    "text": "Remove a folder from the assets system. See man.Assets."
  },
  {
    "title": "RemoveAssetsPackage",
    "href": "api/cpython/functions.html#removeassetspackage",
    "kind": "function",
    "text": "Remove a package from the assets system. See man.Assets."
  },
  {
    "title": "RenderInit",
    "href": "api/cpython/functions.html#renderinit",
    "kind": "function",
    "text": "Initialize the render system. To change the states of the render system afterward use RenderReset."
  },
  {
    "title": "RenderReset",
    "href": "api/cpython/functions.html#renderreset",
    "kind": "function",
    "text": "Change the states of the render system at runtime."
  },
  {
    "title": "RenderResetToWindow",
    "href": "api/cpython/functions.html#renderresettowindow",
    "kind": "function",
    "text": "Resize the renderer backbuffer to the provided window client area dimensions. Return true if a reset was needed and carried out."
  },
  {
    "title": "RenderShutdown",
    "href": "api/cpython/functions.html#rendershutdown",
    "kind": "function",
    "text": "Shutdown the render system."
  },
  {
    "title": "ResetClock",
    "href": "api/cpython/functions.html#resetclock",
    "kind": "function",
    "text": "Reset the elapsed time counter."
  },
  {
    "title": "Reverse",
    "href": "api/cpython/functions.html#reverse",
    "kind": "function",
    "text": "Return the provided vector pointing in the opposite direction."
  },
  {
    "title": "ReverseRotationOrder",
    "href": "api/cpython/functions.html#reverserotationorder",
    "kind": "function",
    "text": "Return the rotation order processing each axis in the reverse order of the input rotation order."
  },
  {
    "title": "Rewind",
    "href": "api/cpython/functions.html#rewind",
    "kind": "function",
    "text": "Rewind the read/write cursor of an open file."
  },
  {
    "title": "RGBA32",
    "href": "api/cpython/functions.html#rgba32",
    "kind": "function",
    "text": "Create a 32 bit integer RGBA color."
  },
  {
    "title": "RmDir",
    "href": "api/cpython/functions.html#rmdir",
    "kind": "function",
    "text": "Remove an empty folder on the local filesystem. See RmTree."
  },
  {
    "title": "RmTree",
    "href": "api/cpython/functions.html#rmtree",
    "kind": "function",
    "text": "Remove a folder on the local filesystem. **Warning:** This function will through all subfolders and erase all files and folders in the target folder."
  },
  {
    "title": "RotationMat2D",
    "href": "api/cpython/functions.html#rotationmat2d",
    "kind": "function",
    "text": "Return a 2D rotation matrix by __a__ radians around the specified __pivot__ point."
  },
  {
    "title": "RotationMat3",
    "href": "api/cpython/functions.html#rotationmat3",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix."
  },
  {
    "title": "RotationMat4",
    "href": "api/cpython/functions.html#rotationmat4",
    "kind": "function",
    "text": "Return a 4x3 rotation matrix from euler angles. The default rotation order is YXZ."
  },
  {
    "title": "RotationMatX",
    "href": "api/cpython/functions.html#rotationmatx",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the world X axis {1, 0, 0}."
  },
  {
    "title": "RotationMatXY",
    "href": "api/cpython/functions.html#rotationmatxy",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the X axis followed by a rotation around the Y axis."
  },
  {
    "title": "RotationMatXYZ",
    "href": "api/cpython/functions.html#rotationmatxyz",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the X axis followed by a rotation around the Y axis then a rotation around the Z axis."
  },
  {
    "title": "RotationMatXZY",
    "href": "api/cpython/functions.html#rotationmatxzy",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the X axis followed by a rotation around the Z axis then a rotation around the Y axis."
  },
  {
    "title": "RotationMatY",
    "href": "api/cpython/functions.html#rotationmaty",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the world Y axis {0, 1, 0}."
  },
  {
    "title": "RotationMatYXZ",
    "href": "api/cpython/functions.html#rotationmatyxz",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the Y axis followed by a rotation around the X axis then a rotation around the Z axis."
  },
  {
    "title": "RotationMatYZX",
    "href": "api/cpython/functions.html#rotationmatyzx",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the Y axis followed by a rotation around the Z axis then a rotation around the X axis."
  },
  {
    "title": "RotationMatZ",
    "href": "api/cpython/functions.html#rotationmatz",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the world Z axis {0, 0, 1}."
  },
  {
    "title": "RotationMatZXY",
    "href": "api/cpython/functions.html#rotationmatzxy",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the Z axis followed by a rotation around the X axis then a rotation around the Y axis."
  },
  {
    "title": "RotationMatZYX",
    "href": "api/cpython/functions.html#rotationmatzyx",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the Z axis followed by a rotation around the Y axis then a rotation around the X axis."
  },
  {
    "title": "SaveBMP",
    "href": "api/cpython/functions.html#savebmp",
    "kind": "function",
    "text": "Save a Picture in [BMP](https://en.wikipedia.org/wiki/BMP_file_format) file format."
  },
  {
    "title": "SaveDataToFile",
    "href": "api/cpython/functions.html#savedatatofile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveFileDialog",
    "href": "api/cpython/functions.html#savefiledialog",
    "kind": "function",
    "text": "Open a native SaveFile dialog."
  },
  {
    "title": "SaveForwardPipelineAAAConfigToFile",
    "href": "api/cpython/functions.html#saveforwardpipelineaaaconfigtofile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveGeometryToFile",
    "href": "api/cpython/functions.html#savegeometrytofile",
    "kind": "function",
    "text": "Save a geometry to the local filesystem. Note that in order to render a geometry it must have been converted to model by the asset compiler. See GeometryBuilder and ModelBuilder."
  },
  {
    "title": "SaveJsonToFile",
    "href": "api/cpython/functions.html#savejsontofile",
    "kind": "function",
    "text": "Save a JSON object to the local filesystem."
  },
  {
    "title": "SavePNG",
    "href": "api/cpython/functions.html#savepng",
    "kind": "function",
    "text": "Save a Picture in [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) file format."
  },
  {
    "title": "SaveSceneBinaryToData",
    "href": "api/cpython/functions.html#savescenebinarytodata",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveSceneBinaryToFile",
    "href": "api/cpython/functions.html#savescenebinarytofile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveSceneJsonToFile",
    "href": "api/cpython/functions.html#savescenejsontofile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveTGA",
    "href": "api/cpython/functions.html#savetga",
    "kind": "function",
    "text": "Save a Picture in [TGA](https://en.wikipedia.org/wiki/Truevision_TGA) file format."
  },
  {
    "title": "ScaleMat3",
    "href": "api/cpython/functions.html#scalemat3",
    "kind": "function",
    "text": "Return a 3x3 scale matrix from a 2D vector."
  },
  {
    "title": "ScaleMat4",
    "href": "api/cpython/functions.html#scalemat4",
    "kind": "function",
    "text": "Return a 4x3 scale matrix from the parameter scaling vector."
  },
  {
    "title": "SceneClearSystems",
    "href": "api/cpython/functions.html#sceneclearsystems",
    "kind": "function",
    "text": "Clear scene and all optional systems."
  },
  {
    "title": "SceneGarbageCollectSystems",
    "href": "api/cpython/functions.html#scenegarbagecollectsystems",
    "kind": "function",
    "text": "Garbage collect a scene and all its optional systems."
  },
  {
    "title": "SceneSyncToSystemsFromAssets",
    "href": "api/cpython/functions.html#scenesynctosystemsfromassets",
    "kind": "function",
    "text": "Synchronize optional systems (eg. physics or script) states with the scene states. Load resources from the assets system if required. See man.Assets."
  },
  {
    "title": "SceneSyncToSystemsFromFile",
    "href": "api/cpython/functions.html#scenesynctosystemsfromfile",
    "kind": "function",
    "text": "Synchronize optional systems (eg. physics or script) states with the scene states. Load resources from the local filesystem if required. See man.Assets."
  },
  {
    "title": "SceneUpdateSystems",
    "href": "api/cpython/functions.html#sceneupdatesystems",
    "kind": "function",
    "text": "Update a scene and all its optional systems."
  },
  {
    "title": "ScreenSpaceToClipSpace",
    "href": "api/cpython/functions.html#screenspacetoclipspace",
    "kind": "function",
    "text": "Transform a screen position to clip space."
  },
  {
    "title": "Sec",
    "href": "api/cpython/functions.html#sec",
    "kind": "function",
    "text": "Convert a value in seconds to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Seed",
    "href": "api/cpython/functions.html#seed",
    "kind": "function",
    "text": "Set the starting seed of the pseudo-random number generator."
  },
  {
    "title": "Seek",
    "href": "api/cpython/functions.html#seek",
    "kind": "function",
    "text": "Move the handle cursor to a specific position in the file."
  },
  {
    "title": "SendVRControllerHapticPulse",
    "href": "api/cpython/functions.html#sendvrcontrollerhapticpulse",
    "kind": "function",
    "text": "Send an haptic pulse to a named VR controller. See GetVRControllerNames."
  },
  {
    "title": "SetAxises",
    "href": "api/cpython/functions.html#setaxises",
    "kind": "function",
    "text": "Inject X, Y and Z axises into a 3x3 matrix."
  },
  {
    "title": "SetColumn",
    "href": "api/cpython/functions.html#setcolumn",
    "kind": "function",
    "text": "Returns the nth column."
  },
  {
    "title": "SetHeight",
    "href": "api/cpython/functions.html#setheight",
    "kind": "function",
    "text": "Set a rectangle height."
  },
  {
    "title": "SetJsonValue",
    "href": "api/cpython/functions.html#setjsonvalue",
    "kind": "function",
    "text": "Set a JSON key value."
  },
  {
    "title": "SetListener",
    "href": "api/cpython/functions.html#setlistener",
    "kind": "function",
    "text": "Set the listener transformation and velocity for spatialization by the audio system."
  },
  {
    "title": "SetLogDetailed",
    "href": "api/cpython/functions.html#setlogdetailed",
    "kind": "function",
    "text": "Display the `details` field of log outputs."
  },
  {
    "title": "SetLogLevel",
    "href": "api/cpython/functions.html#setloglevel",
    "kind": "function",
    "text": "Control which log levels should be displayed. See Log, Warn, Error and Debug."
  },
  {
    "title": "SetMaterialAlphaCut",
    "href": "api/cpython/functions.html#setmaterialalphacut",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialAmbientUsesUV1",
    "href": "api/cpython/functions.html#setmaterialambientusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialBlendMode",
    "href": "api/cpython/functions.html#setmaterialblendmode",
    "kind": "function",
    "text": "Set material blend mode."
  },
  {
    "title": "SetMaterialDepthTest",
    "href": "api/cpython/functions.html#setmaterialdepthtest",
    "kind": "function",
    "text": "Set material depth test."
  },
  {
    "title": "SetMaterialDiffuseUsesUV1",
    "href": "api/cpython/functions.html#setmaterialdiffuseusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialFaceCulling",
    "href": "api/cpython/functions.html#setmaterialfaceculling",
    "kind": "function",
    "text": "Set material face culling."
  },
  {
    "title": "SetMaterialNormalMapInWorldSpace",
    "href": "api/cpython/functions.html#setmaterialnormalmapinworldspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialProgram",
    "href": "api/cpython/functions.html#setmaterialprogram",
    "kind": "function",
    "text": "Set material pipeline program. You should call UpdateMaterialPipelineProgramVariant after changing a material pipeline program so that the correct variant is selected according to the material states."
  },
  {
    "title": "SetMaterialSkinning",
    "href": "api/cpython/functions.html#setmaterialskinning",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialSpecularUsesUV1",
    "href": "api/cpython/functions.html#setmaterialspecularusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialTexture",
    "href": "api/cpython/functions.html#setmaterialtexture",
    "kind": "function",
    "text": "Set a material uniform texture and texture stage. Note: The texture stage specified should match the uniform declaration in the shader program."
  },
  {
    "title": "SetMaterialTextureRef",
    "href": "api/cpython/functions.html#setmaterialtextureref",
    "kind": "function",
    "text": "Set a material uniform texture reference. See PipelineResources."
  },
  {
    "title": "SetMaterialValue",
    "href": "api/cpython/functions.html#setmaterialvalue",
    "kind": "function",
    "text": "Set a material uniform value."
  },
  {
    "title": "SetMaterialWriteRGBA",
    "href": "api/cpython/functions.html#setmaterialwritergba",
    "kind": "function",
    "text": "Set a material color write mask."
  },
  {
    "title": "SetMaterialWriteZ",
    "href": "api/cpython/functions.html#setmaterialwritez",
    "kind": "function",
    "text": "Set a material depth write mask."
  },
  {
    "title": "SetRenderDebug",
    "href": "api/cpython/functions.html#setrenderdebug",
    "kind": "function",
    "text": "Set render system debug flags."
  },
  {
    "title": "SetRow",
    "href": "api/cpython/functions.html#setrow",
    "kind": "function",
    "text": "Sets the nth row of a matrix."
  },
  {
    "title": "SetS",
    "href": "api/cpython/functions.html#sets",
    "kind": "function",
    "text": "Shortcut for SetScale."
  },
  {
    "title": "SetSaturation",
    "href": "api/cpython/functions.html#setsaturation",
    "kind": "function",
    "text": "Return a copy of the input RGBA color with its saturation set to the specified value, alpha channel is left unmodified. See ToHLS and FromHLS."
  },
  {
    "title": "SetScale",
    "href": "api/cpython/functions.html#setscale",
    "kind": "function",
    "text": "Set the scaling part of the transformation matrix."
  },
  {
    "title": "SetSourcePanning",
    "href": "api/cpython/functions.html#setsourcepanning",
    "kind": "function",
    "text": "Set a playing audio source panning."
  },
  {
    "title": "SetSourceRepeat",
    "href": "api/cpython/functions.html#setsourcerepeat",
    "kind": "function",
    "text": "Set audio source repeat mode."
  },
  {
    "title": "SetSourceTimecode",
    "href": "api/cpython/functions.html#setsourcetimecode",
    "kind": "function",
    "text": "Set timecode of the audio source."
  },
  {
    "title": "SetSourceTransform",
    "href": "api/cpython/functions.html#setsourcetransform",
    "kind": "function",
    "text": "Set a playing spatialized audio source transformation."
  },
  {
    "title": "SetSourceVolume",
    "href": "api/cpython/functions.html#setsourcevolume",
    "kind": "function",
    "text": "Set audio source volume."
  },
  {
    "title": "SetT",
    "href": "api/cpython/functions.html#sett",
    "kind": "function",
    "text": "Shortcut for SetTranslation."
  },
  {
    "title": "SetTransform",
    "href": "api/cpython/functions.html#settransform",
    "kind": "function",
    "text": "Set the model matrix for the next drawn primitive. If not called, model will be rendered with the identity model matrix."
  },
  {
    "title": "SetTranslation",
    "href": "api/cpython/functions.html#settranslation",
    "kind": "function",
    "text": "Sets the 2D translation part, i.e. the first 2 elements of the last matrix row."
  },
  {
    "title": "SetView2D",
    "href": "api/cpython/functions.html#setview2d",
    "kind": "function",
    "text": "High-level wrapper function to setup a view for 2D rendering. This function calls SetViewClear, SetViewRect then SetViewTransform."
  },
  {
    "title": "SetViewClear",
    "href": "api/cpython/functions.html#setviewclear",
    "kind": "function",
    "text": "Set a view clear parameters. See man.Views."
  },
  {
    "title": "SetViewFrameBuffer",
    "href": "api/cpython/functions.html#setviewframebuffer",
    "kind": "function",
    "text": "Set view output framebuffer. See man.Views."
  },
  {
    "title": "SetViewMode",
    "href": "api/cpython/functions.html#setviewmode",
    "kind": "function",
    "text": "Set view draw ordering mode."
  },
  {
    "title": "SetViewOrthographic",
    "href": "api/cpython/functions.html#setvieworthographic",
    "kind": "function",
    "text": "High-level wrapper function to setup a view for 3D orthographic rendering. This function calls SetViewClear, SetViewRect then SetViewTransform."
  },
  {
    "title": "SetViewPerspective",
    "href": "api/cpython/functions.html#setviewperspective",
    "kind": "function",
    "text": "High-level wrapper function to setup a view for 3D perspective rendering. This function calls SetViewClear, SetViewRect then SetViewTransform."
  },
  {
    "title": "SetViewRect",
    "href": "api/cpython/functions.html#setviewrect",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetViewTransform",
    "href": "api/cpython/functions.html#setviewtransform",
    "kind": "function",
    "text": "Set view transforms, namely the view and projection matrices."
  },
  {
    "title": "SetWidth",
    "href": "api/cpython/functions.html#setwidth",
    "kind": "function",
    "text": "Set a rectangle width."
  },
  {
    "title": "SetWindowClientSize",
    "href": "api/cpython/functions.html#setwindowclientsize",
    "kind": "function",
    "text": "Set the window client size. The client area of a window excludes its decoration."
  },
  {
    "title": "SetWindowPos",
    "href": "api/cpython/functions.html#setwindowpos",
    "kind": "function",
    "text": "Set window position."
  },
  {
    "title": "SetWindowTitle",
    "href": "api/cpython/functions.html#setwindowtitle",
    "kind": "function",
    "text": "Set window title."
  },
  {
    "title": "SetX",
    "href": "api/cpython/functions.html#setx",
    "kind": "function",
    "text": "Sets the first row."
  },
  {
    "title": "SetY",
    "href": "api/cpython/functions.html#sety",
    "kind": "function",
    "text": "Sets the second row."
  },
  {
    "title": "SetZ",
    "href": "api/cpython/functions.html#setz",
    "kind": "function",
    "text": "Sets the third row."
  },
  {
    "title": "ShowCursor",
    "href": "api/cpython/functions.html#showcursor",
    "kind": "function",
    "text": "Show the system mouse cursor. See HideCursor."
  },
  {
    "title": "Sign",
    "href": "api/cpython/functions.html#sign",
    "kind": "function",
    "text": "Returns a vector whose elements are -1 if the corresponding vector element is = 0."
  },
  {
    "title": "SkipClock",
    "href": "api/cpython/functions.html#skipclock",
    "kind": "function",
    "text": "Skip elapsed time since the last call to TickClock."
  },
  {
    "title": "Sleep",
    "href": "api/cpython/functions.html#sleep",
    "kind": "function",
    "text": "Sleep the caller thread, this function will resume execution after waiting for at least the specified amount of time."
  },
  {
    "title": "Slerp",
    "href": "api/cpython/functions.html#slerp",
    "kind": "function",
    "text": "Interpolate between the rotation represented by two quaternions. The _Spherical Linear Interpolation_ will always take the shortest path between the two rotations."
  },
  {
    "title": "SRanipalGetState",
    "href": "api/cpython/functions.html#sranipalgetstate",
    "kind": "function",
    "text": "Return the current SRanipal device state."
  },
  {
    "title": "SRanipalInit",
    "href": "api/cpython/functions.html#sranipalinit",
    "kind": "function",
    "text": "Initial the SRanipal eye detection SDK."
  },
  {
    "title": "SRanipalIsViveProEye",
    "href": "api/cpython/functions.html#sranipalisviveproeye",
    "kind": "function",
    "text": "Return `true` if the eye detection device in use is Vive Pro Eye."
  },
  {
    "title": "SRanipalLaunchEyeCalibration",
    "href": "api/cpython/functions.html#sranipallauncheyecalibration",
    "kind": "function",
    "text": "Launch the eye detection calibration sequence."
  },
  {
    "title": "SRanipalShutdown",
    "href": "api/cpython/functions.html#sranipalshutdown",
    "kind": "function",
    "text": "Shutdown the SRanipal eye detection SDK."
  },
  {
    "title": "StopAllSources",
    "href": "api/cpython/functions.html#stopallsources",
    "kind": "function",
    "text": "Stop all playing audio sources."
  },
  {
    "title": "StopSource",
    "href": "api/cpython/functions.html#stopsource",
    "kind": "function",
    "text": "Stop a playing audio source."
  },
  {
    "title": "StreamOGGAssetSpatialized",
    "href": "api/cpython/functions.html#streamoggassetspatialized",
    "kind": "function",
    "text": ""
  },
  {
    "title": "StreamOGGAssetStereo",
    "href": "api/cpython/functions.html#streamoggassetstereo",
    "kind": "function",
    "text": ""
  },
  {
    "title": "StreamOGGFileSpatialized",
    "href": "api/cpython/functions.html#streamoggfilespatialized",
    "kind": "function",
    "text": ""
  },
  {
    "title": "StreamOGGFileStereo",
    "href": "api/cpython/functions.html#streamoggfilestereo",
    "kind": "function",
    "text": ""
  },
  {
    "title": "StreamWAVAssetSpatialized",
    "href": "api/cpython/functions.html#streamwavassetspatialized",
    "kind": "function",
    "text": "Start an audio stream from a WAV file from the assets system. See SetSourceTransform and man.Assets."
  },
  {
    "title": "StreamWAVAssetStereo",
    "href": "api/cpython/functions.html#streamwavassetstereo",
    "kind": "function",
    "text": "Start an audio stream from a WAV file from the assets system. See man.Assets."
  },
  {
    "title": "StreamWAVFileSpatialized",
    "href": "api/cpython/functions.html#streamwavfilespatialized",
    "kind": "function",
    "text": "Start an audio stream from a WAV file on the local filesystem. See SetSourceTransform."
  },
  {
    "title": "StreamWAVFileStereo",
    "href": "api/cpython/functions.html#streamwavfilestereo",
    "kind": "function",
    "text": "Start an audio stream from a WAV file on the local filesystem. See man.Assets."
  },
  {
    "title": "StringToFile",
    "href": "api/cpython/functions.html#stringtofile",
    "kind": "function",
    "text": "Return the content of a file on the local filesystem as a string."
  },
  {
    "title": "SubmitSceneToForwardPipeline",
    "href": "api/cpython/functions.html#submitscenetoforwardpipeline",
    "kind": "function",
    "text": "Submit a scene to a forward pipeline. See PrepareSceneForwardPipelineCommonRenderData and PrepareSceneForwardPipelineViewDependentRenderData if you need to render the same scene from different points of view."
  },
  {
    "title": "SubmitSceneToPipeline",
    "href": "api/cpython/functions.html#submitscenetopipeline",
    "kind": "function",
    "text": "See SubmitSceneToForwardPipeline."
  },
  {
    "title": "SwapFileExtension",
    "href": "api/cpython/functions.html#swapfileextension",
    "kind": "function",
    "text": "Return the input file path with its extension replaced."
  },
  {
    "title": "Tell",
    "href": "api/cpython/functions.html#tell",
    "kind": "function",
    "text": "Return the current handle cursor position in bytes."
  },
  {
    "title": "TestVisibility",
    "href": "api/cpython/functions.html#testvisibility",
    "kind": "function",
    "text": "Test if a list of 3d points are inside or outside a Frustum."
  },
  {
    "title": "TickClock",
    "href": "api/cpython/functions.html#tickclock",
    "kind": "function",
    "text": "Advance the engine clock and return the elapsed time since the last call to this function. See GetClock to retrieve the current clock. See GetClockDt."
  },
  {
    "title": "time_from_day",
    "href": "api/cpython/functions.html#time_from_day",
    "kind": "function",
    "text": "Convert days to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_hour",
    "href": "api/cpython/functions.html#time_from_hour",
    "kind": "function",
    "text": "Convert hours to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_min",
    "href": "api/cpython/functions.html#time_from_min",
    "kind": "function",
    "text": "Convert minutes to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_ms",
    "href": "api/cpython/functions.html#time_from_ms",
    "kind": "function",
    "text": "Convert milliseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_ms_f",
    "href": "api/cpython/functions.html#time_from_ms_f",
    "kind": "function",
    "text": "Convert milliseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_ns",
    "href": "api/cpython/functions.html#time_from_ns",
    "kind": "function",
    "text": "Convert nanoseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_sec",
    "href": "api/cpython/functions.html#time_from_sec",
    "kind": "function",
    "text": "Convert seconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_sec_f",
    "href": "api/cpython/functions.html#time_from_sec_f",
    "kind": "function",
    "text": "Convert fractional seconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_us",
    "href": "api/cpython/functions.html#time_from_us",
    "kind": "function",
    "text": "Convert microseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_us_f",
    "href": "api/cpython/functions.html#time_from_us_f",
    "kind": "function",
    "text": "Convert fractional microseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_now",
    "href": "api/cpython/functions.html#time_now",
    "kind": "function",
    "text": "Return the current system time."
  },
  {
    "title": "time_to_day",
    "href": "api/cpython/functions.html#time_to_day",
    "kind": "function",
    "text": "Convert time to days."
  },
  {
    "title": "time_to_hour",
    "href": "api/cpython/functions.html#time_to_hour",
    "kind": "function",
    "text": "Convert time to hours."
  },
  {
    "title": "time_to_min",
    "href": "api/cpython/functions.html#time_to_min",
    "kind": "function",
    "text": "Convert time to minutes."
  },
  {
    "title": "time_to_ms",
    "href": "api/cpython/functions.html#time_to_ms",
    "kind": "function",
    "text": "Convert time to milliseconds."
  },
  {
    "title": "time_to_ms_f",
    "href": "api/cpython/functions.html#time_to_ms_f",
    "kind": "function",
    "text": "Convert time to miliseconds."
  },
  {
    "title": "time_to_ns",
    "href": "api/cpython/functions.html#time_to_ns",
    "kind": "function",
    "text": "Convert time to nanoseconds."
  },
  {
    "title": "time_to_sec",
    "href": "api/cpython/functions.html#time_to_sec",
    "kind": "function",
    "text": "Convert time to seconds."
  },
  {
    "title": "time_to_sec_f",
    "href": "api/cpython/functions.html#time_to_sec_f",
    "kind": "function",
    "text": "Convert time to fractional seconds."
  },
  {
    "title": "time_to_string",
    "href": "api/cpython/functions.html#time_to_string",
    "kind": "function",
    "text": "Return time as a human-readable string."
  },
  {
    "title": "time_to_us",
    "href": "api/cpython/functions.html#time_to_us",
    "kind": "function",
    "text": "Convert time to microseconds."
  },
  {
    "title": "time_to_us_f",
    "href": "api/cpython/functions.html#time_to_us_f",
    "kind": "function",
    "text": "Convert time to fractional microseconds."
  },
  {
    "title": "ToEuler",
    "href": "api/cpython/functions.html#toeuler",
    "kind": "function",
    "text": "Convert a quaternion rotation to its _Euler_ vector representation."
  },
  {
    "title": "ToFloatRect",
    "href": "api/cpython/functions.html#tofloatrect",
    "kind": "function",
    "text": "Return an integer rectangle as a floating point rectangle."
  },
  {
    "title": "ToHLS",
    "href": "api/cpython/functions.html#tohls",
    "kind": "function",
    "text": "Convert input RGBA color to hue/luminance/saturation, alpha channel is left unmodified."
  },
  {
    "title": "ToIntRect",
    "href": "api/cpython/functions.html#tointrect",
    "kind": "function",
    "text": "Return a floating point rectangle as an integer rectangle."
  },
  {
    "title": "ToMatrix3",
    "href": "api/cpython/functions.html#tomatrix3",
    "kind": "function",
    "text": "Convert a quaternion rotation to its Mat3 representation."
  },
  {
    "title": "Touch",
    "href": "api/cpython/functions.html#touch",
    "kind": "function",
    "text": "Submit an empty primitive to the view. See Frame."
  },
  {
    "title": "TransformationMat4",
    "href": "api/cpython/functions.html#transformationmat4",
    "kind": "function",
    "text": "Creates a 4x3 transformation matrix from the translation vector __p__, the 3x3 rotation Matrix __m__ (or YXZ euler rotation vector __e__) and the scaling vector __s__. This is a more efficient version of `TranslationMat4(p) * ScaleMat4(s) *"
  },
  {
    "title": "TransformFrustum",
    "href": "api/cpython/functions.html#transformfrustum",
    "kind": "function",
    "text": "Return the input frustum transformed by the provided world matrix."
  },
  {
    "title": "TranslationMat3",
    "href": "api/cpython/functions.html#translationmat3",
    "kind": "function",
    "text": "Return a 2D translation 3x3 matrix from the first 2 components (__x__,__y__) of the parameter vector."
  },
  {
    "title": "TranslationMat4",
    "href": "api/cpython/functions.html#translationmat4",
    "kind": "function",
    "text": "Return a 4x3 translation matrix from the parameter displacement vector."
  },
  {
    "title": "Transpose",
    "href": "api/cpython/functions.html#transpose",
    "kind": "function",
    "text": "Return the transpose of the input matrix. For a pure rotation matrix this returns the opposite transformation so that M*M T =I."
  },
  {
    "title": "Union",
    "href": "api/cpython/functions.html#union",
    "kind": "function",
    "text": "Compute the union of this bounding volume with another volume or a 3d position."
  },
  {
    "title": "Unlink",
    "href": "api/cpython/functions.html#unlink",
    "kind": "function",
    "text": "Remove a file from the local filesystem."
  },
  {
    "title": "UnloadSound",
    "href": "api/cpython/functions.html#unloadsound",
    "kind": "function",
    "text": "Unload a sound from the audio system."
  },
  {
    "title": "UnprojectFromClipSpace",
    "href": "api/cpython/functions.html#unprojectfromclipspace",
    "kind": "function",
    "text": "Unproject a clip space position to view space."
  },
  {
    "title": "UnprojectFromScreenSpace",
    "href": "api/cpython/functions.html#unprojectfromscreenspace",
    "kind": "function",
    "text": "Unproject a screen space position to view space."
  },
  {
    "title": "UnprojectOrthoFromClipSpace",
    "href": "api/cpython/functions.html#unprojectorthofromclipspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "UnprojectOrthoFromScreenSpace",
    "href": "api/cpython/functions.html#unprojectorthofromscreenspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "UpdateMaterialPipelineProgramVariant",
    "href": "api/cpython/functions.html#updatematerialpipelineprogramvariant",
    "kind": "function",
    "text": "Select the proper pipeline program variant for the current material state."
  },
  {
    "title": "UpdateTexture",
    "href": "api/cpython/functions.html#updatetexture",
    "kind": "function",
    "text": ""
  },
  {
    "title": "UpdateTextureFromPicture",
    "href": "api/cpython/functions.html#updatetexturefrompicture",
    "kind": "function",
    "text": "Update texture content from the provided picture. Note: The picture is expected to be in a format compatible with the texture format."
  },
  {
    "title": "UpdateWindow",
    "href": "api/cpython/functions.html#updatewindow",
    "kind": "function",
    "text": "Update a window on the host system."
  },
  {
    "title": "Vec3I",
    "href": "api/cpython/functions.html#vec3i",
    "kind": "function",
    "text": "Create a vector from integer values in the [0;255] range."
  },
  {
    "title": "Vec4I",
    "href": "api/cpython/functions.html#vec4i",
    "kind": "function",
    "text": "Create a vector from integer values in the [0;255] range."
  },
  {
    "title": "VectorMat3",
    "href": "api/cpython/functions.html#vectormat3",
    "kind": "function",
    "text": "Return a vector as a matrix."
  },
  {
    "title": "VertexLayoutPosFloatColorFloat",
    "href": "api/cpython/functions.html#vertexlayoutposfloatcolorfloat",
    "kind": "function",
    "text": ""
  },
  {
    "title": "VertexLayoutPosFloatColorUInt8",
    "href": "api/cpython/functions.html#vertexlayoutposfloatcoloruint8",
    "kind": "function",
    "text": ""
  },
  {
    "title": "VertexLayoutPosFloatNormFloat",
    "href": "api/cpython/functions.html#vertexlayoutposfloatnormfloat",
    "kind": "function",
    "text": "Simple vertex layout with float position and normal. ```python vtx_layout = VertexLayout() vtx_layout.Begin() vtx_layout.Add(hg.A_Position, 3, hg.AT_Float) vtx_layout.Add(hg.A_Normal, 3, hg.AT_Float) vtx_layout.End() ```"
  },
  {
    "title": "VertexLayoutPosFloatNormUInt8",
    "href": "api/cpython/functions.html#vertexlayoutposfloatnormuint8",
    "kind": "function",
    "text": "Simple vertex layout with float position and 8-bit unsigned integer normal. ```python vtx_layout = VertexLayout() vtx_layout.Begin() vtx_layout.Add(hg.A_Position, 3, hg.AT_Float) vtx_layout.Add(hg.A_Normal, 3, hg.AT_Uint8, True, True) vtx_l"
  },
  {
    "title": "VertexLayoutPosFloatNormUInt8TexCoord0UInt8",
    "href": "api/cpython/functions.html#vertexlayoutposfloatnormuint8texcoord0uint8",
    "kind": "function",
    "text": ""
  },
  {
    "title": "VertexLayoutPosFloatTexCoord0UInt8",
    "href": "api/cpython/functions.html#vertexlayoutposfloattexcoord0uint8",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Warn",
    "href": "api/cpython/functions.html#warn",
    "kind": "function",
    "text": ""
  },
  {
    "title": "WindowHasFocus",
    "href": "api/cpython/functions.html#windowhasfocus",
    "kind": "function",
    "text": "Return `true` if the provided window has focus, `false` otherwise."
  },
  {
    "title": "WindowSystemInit",
    "href": "api/cpython/functions.html#windowsysteminit",
    "kind": "function",
    "text": "Initialize the Window system."
  },
  {
    "title": "WindowSystemShutdown",
    "href": "api/cpython/functions.html#windowsystemshutdown",
    "kind": "function",
    "text": "Shutdown the window system. See WindowSystemInit."
  },
  {
    "title": "Wrap",
    "href": "api/cpython/functions.html#wrap",
    "kind": "function",
    "text": "Wrap the input value so that it fits in the specified inclusive range."
  },
  {
    "title": "WriteFloat",
    "href": "api/cpython/functions.html#writefloat",
    "kind": "function",
    "text": "Write a binary 32 bit floating point value to a file."
  },
  {
    "title": "WriteString",
    "href": "api/cpython/functions.html#writestring",
    "kind": "function",
    "text": "Write a string to a file as 32 bit integer size followed by the string content in UTF8."
  },
  {
    "title": "WriteUInt16",
    "href": "api/cpython/functions.html#writeuint16",
    "kind": "function",
    "text": "Write a binary 16 bit unsigned integer to a file."
  },
  {
    "title": "WriteUInt32",
    "href": "api/cpython/functions.html#writeuint32",
    "kind": "function",
    "text": "Write a binary 32 bit unsigned integer to a file."
  },
  {
    "title": "WriteUInt8",
    "href": "api/cpython/functions.html#writeuint8",
    "kind": "function",
    "text": "Write a binary 8 bit unsigned integer to a file."
  },
  {
    "title": "ZoomFactorToFov",
    "href": "api/cpython/functions.html#zoomfactortofov",
    "kind": "function",
    "text": "Convert from a zoom factor value in meters to a fov value in radian."
  },
  {
    "title": "AnimLoopMode",
    "href": "api/cpython/constants.html#animloopmode",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Attrib",
    "href": "api/cpython/constants.html#attrib",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "AttribType",
    "href": "api/cpython/constants.html#attribtype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "AudioFrameFormat",
    "href": "api/cpython/constants.html#audioframeformat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Axis",
    "href": "api/cpython/constants.html#axis",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "BackbufferRatio",
    "href": "api/cpython/constants.html#backbufferratio",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "BlendMode",
    "href": "api/cpython/constants.html#blendmode",
    "kind": "enumeration",
    "text": "Control the compositing mode used to draw primitives."
  },
  {
    "title": "CollisionEventTrackingMode",
    "href": "api/cpython/constants.html#collisioneventtrackingmode",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "CollisionType",
    "href": "api/cpython/constants.html#collisiontype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "DepthTest",
    "href": "api/cpython/constants.html#depthtest",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "DirEntryType",
    "href": "api/cpython/constants.html#direntrytype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "DrawTextHAlign",
    "href": "api/cpython/constants.html#drawtexthalign",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "DrawTextVAlign",
    "href": "api/cpython/constants.html#drawtextvalign",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Easing",
    "href": "api/cpython/constants.html#easing",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "FaceCulling",
    "href": "api/cpython/constants.html#faceculling",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ForwardPipelineAAADebugBuffer",
    "href": "api/cpython/constants.html#forwardpipelineaaadebugbuffer",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ForwardPipelineLightType",
    "href": "api/cpython/constants.html#forwardpipelinelighttype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ForwardPipelineShadowType",
    "href": "api/cpython/constants.html#forwardpipelineshadowtype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "GamepadAxes",
    "href": "api/cpython/constants.html#gamepadaxes",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "GamepadButton",
    "href": "api/cpython/constants.html#gamepadbutton",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "HandsSide",
    "href": "api/cpython/constants.html#handsside",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImDrawFlags",
    "href": "api/cpython/constants.html#imdrawflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiCol",
    "href": "api/cpython/constants.html#imguicol",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiColorEditFlags",
    "href": "api/cpython/constants.html#imguicoloreditflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiComboFlags",
    "href": "api/cpython/constants.html#imguicomboflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiCond",
    "href": "api/cpython/constants.html#imguicond",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiFocusedFlags",
    "href": "api/cpython/constants.html#imguifocusedflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiHoveredFlags",
    "href": "api/cpython/constants.html#imguihoveredflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiInputTextFlags",
    "href": "api/cpython/constants.html#imguiinputtextflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiMouseButton",
    "href": "api/cpython/constants.html#imguimousebutton",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiPopupFlags",
    "href": "api/cpython/constants.html#imguipopupflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiSelectableFlags",
    "href": "api/cpython/constants.html#imguiselectableflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiStyleVar",
    "href": "api/cpython/constants.html#imguistylevar",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiTreeNodeFlags",
    "href": "api/cpython/constants.html#imguitreenodeflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiWindowFlags",
    "href": "api/cpython/constants.html#imguiwindowflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Key",
    "href": "api/cpython/constants.html#key",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "LightShadowType",
    "href": "api/cpython/constants.html#lightshadowtype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "LightType",
    "href": "api/cpython/constants.html#lighttype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "LogLevel",
    "href": "api/cpython/constants.html#loglevel",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "MonitorRotation",
    "href": "api/cpython/constants.html#monitorrotation",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "MouseButton",
    "href": "api/cpython/constants.html#mousebutton",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "NodeComponentIdx",
    "href": "api/cpython/constants.html#nodecomponentidx",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "OpenVRAA",
    "href": "api/cpython/constants.html#openvraa",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "OpenXRAA",
    "href": "api/cpython/constants.html#openxraa",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "OpenXRExtensions",
    "href": "api/cpython/constants.html#openxrextensions",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "PictureFormat",
    "href": "api/cpython/constants.html#pictureformat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "RendererType",
    "href": "api/cpython/constants.html#renderertype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "RigidBodyType",
    "href": "api/cpython/constants.html#rigidbodytype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "RotationOrder",
    "href": "api/cpython/constants.html#rotationorder",
    "kind": "enumeration",
    "text": "This enumeration is used to control the order of rotation around the X, Y and Z axises."
  },
  {
    "title": "SceneForwardPipelinePass",
    "href": "api/cpython/constants.html#sceneforwardpipelinepass",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "SeekMode",
    "href": "api/cpython/constants.html#seekmode",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "SourceRepeat",
    "href": "api/cpython/constants.html#sourcerepeat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "SourceState",
    "href": "api/cpython/constants.html#sourcestate",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "TextureFormat",
    "href": "api/cpython/constants.html#textureformat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "VideoFrameFormat",
    "href": "api/cpython/constants.html#videoframeformat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ViewMode",
    "href": "api/cpython/constants.html#viewmode",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Visibility",
    "href": "api/cpython/constants.html#visibility",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "VRControllerButton",
    "href": "api/cpython/constants.html#vrcontrollerbutton",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "WindowVisibility",
    "href": "api/cpython/constants.html#windowvisibility",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "XrHandJoint",
    "href": "api/cpython/constants.html#xrhandjoint",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ClearFlags",
    "href": "api/cpython/constants.html#clearflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "DebugFlags",
    "href": "api/cpython/constants.html#debugflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "LoadSaveSceneFlags",
    "href": "api/cpython/constants.html#loadsavesceneflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "ResetFlags",
    "href": "api/cpython/constants.html#resetflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "SoundRef",
    "href": "api/cpython/constants.html#soundref",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "SourceRef",
    "href": "api/cpython/constants.html#sourceref",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "TextureFlags",
    "href": "api/cpython/constants.html#textureflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "Bloom",
    "href": "api/lua/classes.html#bloom",
    "kind": "class",
    "text": "Bloom post-process object holding internal states and resources. Create with CreateBloomFromAssets or CreateBloomFromFile, use with ApplyBloom, finally call DestroyBloom to dispose of resources when done."
  },
  {
    "title": "btGeneric6DofConstraint",
    "href": "api/lua/classes.html#btgeneric6dofconstraint",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Camera",
    "href": "api/lua/classes.html#camera",
    "kind": "class",
    "text": "Add this component to a Node to implement the camera aspect. Create a camera component with Scene_CreateCamera, use CreateCamera to create a complete camera node."
  },
  {
    "title": "CameraZRange",
    "href": "api/lua/classes.html#camerazrange",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Canvas",
    "href": "api/lua/classes.html#canvas",
    "kind": "class",
    "text": "Holds the canvas properties of a scene, see the `canvas` member of class Scene."
  },
  {
    "title": "Collision",
    "href": "api/lua/classes.html#collision",
    "kind": "class",
    "text": "Collision component, see man.Physics."
  },
  {
    "title": "Color",
    "href": "api/lua/classes.html#color",
    "kind": "class",
    "text": "Four-component RGBA color object."
  },
  {
    "title": "ColorList",
    "href": "api/lua/classes.html#colorlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Contact",
    "href": "api/lua/classes.html#contact",
    "kind": "class",
    "text": "Object containing the world space position, normal and depth of a contact as reported by the collision system."
  },
  {
    "title": "ContactList",
    "href": "api/lua/classes.html#contactlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Crowd",
    "href": "api/lua/classes.html#crowd",
    "kind": "class",
    "text": "A group of navigation agents to efficiently simulate a crowd, see man.Navigation."
  },
  {
    "title": "CrowdAgent",
    "href": "api/lua/classes.html#crowdagent",
    "kind": "class",
    "text": "State of a crowd agent."
  },
  {
    "title": "CrowdAgentParams",
    "href": "api/lua/classes.html#crowdagentparams",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Data",
    "href": "api/lua/classes.html#data",
    "kind": "class",
    "text": ""
  },
  {
    "title": "DearImguiContext",
    "href": "api/lua/classes.html#dearimguicontext",
    "kind": "class",
    "text": "Context to render immediate GUI."
  },
  {
    "title": "DirEntry",
    "href": "api/lua/classes.html#direntry",
    "kind": "class",
    "text": ""
  },
  {
    "title": "DirEntryList",
    "href": "api/lua/classes.html#direntrylist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "draw_sceneCallback",
    "href": "api/lua/classes.html#draw_scenecallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Environment",
    "href": "api/lua/classes.html#environment",
    "kind": "class",
    "text": "Environment properties of a scene, see `environment` member of the Scene class."
  },
  {
    "title": "File",
    "href": "api/lua/classes.html#file",
    "kind": "class",
    "text": "Interface to a file on the host local filesystem."
  },
  {
    "title": "FileFilter",
    "href": "api/lua/classes.html#filefilter",
    "kind": "class",
    "text": ""
  },
  {
    "title": "FileFilterList",
    "href": "api/lua/classes.html#filefilterlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Font",
    "href": "api/lua/classes.html#font",
    "kind": "class",
    "text": "Font object for realtime rendering."
  },
  {
    "title": "ForwardPipeline",
    "href": "api/lua/classes.html#forwardpipeline",
    "kind": "class",
    "text": "Rendering pipeline implementing a forward rendering strategy. The main characteristics of this pipeline are: - Render in two passes: opaque display lists then transparent ones. - Fixed 8 light slots supporting 1 linear light with PSSM shado"
  },
  {
    "title": "ForwardPipelineAAA",
    "href": "api/lua/classes.html#forwardpipelineaaa",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ForwardPipelineAAAConfig",
    "href": "api/lua/classes.html#forwardpipelineaaaconfig",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ForwardPipelineFog",
    "href": "api/lua/classes.html#forwardpipelinefog",
    "kind": "class",
    "text": "Fog properties for the forward pipeline."
  },
  {
    "title": "ForwardPipelineLight",
    "href": "api/lua/classes.html#forwardpipelinelight",
    "kind": "class",
    "text": "Single light for the forward pipeline. The complete lighting rig is passed as a ForwardPipelineLights, see PrepareForwardPipelineLights."
  },
  {
    "title": "ForwardPipelineLightList",
    "href": "api/lua/classes.html#forwardpipelinelightlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ForwardPipelineLights",
    "href": "api/lua/classes.html#forwardpipelinelights",
    "kind": "class",
    "text": ""
  },
  {
    "title": "FrameBuffer",
    "href": "api/lua/classes.html#framebuffer",
    "kind": "class",
    "text": ""
  },
  {
    "title": "FrameBufferHandle",
    "href": "api/lua/classes.html#framebufferhandle",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Frustum",
    "href": "api/lua/classes.html#frustum",
    "kind": "class",
    "text": "A view frustum, perspective or orthographic, holding the necessary information to perform culling queries. It can be used to test wether a volume is inside or outside the frustum it represents."
  },
  {
    "title": "Gamepad",
    "href": "api/lua/classes.html#gamepad",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetGamepadNames to query for available gamepad devices."
  },
  {
    "title": "GamepadState",
    "href": "api/lua/classes.html#gamepadstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Geometry",
    "href": "api/lua/classes.html#geometry",
    "kind": "class",
    "text": "Base geometry object. Before a geometry can be displayed, it must be converted to Model by the asset compiler (see man.AssetCompiler). To programmatically create a geometry use GeometryBuilder."
  },
  {
    "title": "GeometryBuilder",
    "href": "api/lua/classes.html#geometrybuilder",
    "kind": "class",
    "text": "Use the geometry builder to programmatically create geometries. No optimization are performed by the geometry builder on the input data. To programmatically build a geometry for immediate display see ModelBuilder to directly build models."
  },
  {
    "title": "ImDrawList",
    "href": "api/lua/classes.html#imdrawlist",
    "kind": "class",
    "text": "Immediate GUI drawing list. This object can be used to perform custom drawing operations on top of an imgui window."
  },
  {
    "title": "ImFont",
    "href": "api/lua/classes.html#imfont",
    "kind": "class",
    "text": "Immediate GUI font."
  },
  {
    "title": "Instance",
    "href": "api/lua/classes.html#instance",
    "kind": "class",
    "text": "Component to instantiate a scene as a child of a node upon setup."
  },
  {
    "title": "intList",
    "href": "api/lua/classes.html#intlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "IntRect",
    "href": "api/lua/classes.html#intrect",
    "kind": "class",
    "text": ""
  },
  {
    "title": "IsoSurface",
    "href": "api/lua/classes.html#isosurface",
    "kind": "class",
    "text": "An iso-surface represents points of a constant value within a volume of space. This class holds a fixed-size 3-dimensional grid of values that can efficiently be converted to a Model at runtime."
  },
  {
    "title": "iVec2",
    "href": "api/lua/classes.html#ivec2",
    "kind": "class",
    "text": "2-dimensional integer vector."
  },
  {
    "title": "iVec2List",
    "href": "api/lua/classes.html#ivec2list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "IVideoStreamer",
    "href": "api/lua/classes.html#ivideostreamer",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Joystick",
    "href": "api/lua/classes.html#joystick",
    "kind": "class",
    "text": ""
  },
  {
    "title": "JoystickState",
    "href": "api/lua/classes.html#joystickstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "JSON",
    "href": "api/lua/classes.html#json",
    "kind": "class",
    "text": "JSON read/write object."
  },
  {
    "title": "Keyboard",
    "href": "api/lua/classes.html#keyboard",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetKeyboardNames to query for available keyboard devices."
  },
  {
    "title": "KeyboardState",
    "href": "api/lua/classes.html#keyboardstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Light",
    "href": "api/lua/classes.html#light",
    "kind": "class",
    "text": "Add this component to a node to turn it into a light source, see man.ForwardPipeline."
  },
  {
    "title": "LuaObject",
    "href": "api/lua/classes.html#luaobject",
    "kind": "class",
    "text": "Opaque reference to an Lua object. This type is used to transfer values between VMs, see man.Scripting."
  },
  {
    "title": "LuaObjectList",
    "href": "api/lua/classes.html#luaobjectlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Mat3",
    "href": "api/lua/classes.html#mat3",
    "kind": "class",
    "text": "A 3x3 matrix used to store rotation."
  },
  {
    "title": "Mat4",
    "href": "api/lua/classes.html#mat4",
    "kind": "class",
    "text": "A 3x4 matrix used to store complete transformation including rotation, scale and position."
  },
  {
    "title": "Mat44",
    "href": "api/lua/classes.html#mat44",
    "kind": "class",
    "text": "A 4x4 matrix used to store projection matrices."
  },
  {
    "title": "Mat4List",
    "href": "api/lua/classes.html#mat4list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Material",
    "href": "api/lua/classes.html#material",
    "kind": "class",
    "text": "High-level description of visual aspects of a surface. A material is comprised of a PipelineProgramRef, per-uniform value or texture, and a RenderState. See man.ForwardPipeline and man.PipelineShader."
  },
  {
    "title": "MaterialList",
    "href": "api/lua/classes.html#materiallist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "MaterialRef",
    "href": "api/lua/classes.html#materialref",
    "kind": "class",
    "text": "Reference to a Material inside a PipelineResources object."
  },
  {
    "title": "MinMax",
    "href": "api/lua/classes.html#minmax",
    "kind": "class",
    "text": "3D bounding volume defined by a minimum and maximum position."
  },
  {
    "title": "Model",
    "href": "api/lua/classes.html#model",
    "kind": "class",
    "text": "Runtime version of a Geometry. A model can be drawn to screen by calling DrawModel or by assigning it to the Object component of a node. To programmatically create a model see ModelBuilder."
  },
  {
    "title": "ModelBuilder",
    "href": "api/lua/classes.html#modelbuilder",
    "kind": "class",
    "text": "Use the model builder to programmatically build models at runtime. The input data is optimized upon submission."
  },
  {
    "title": "ModelRef",
    "href": "api/lua/classes.html#modelref",
    "kind": "class",
    "text": "Reference to a Model inside a PipelineResources object. See LoadModelFromFile, LoadModelFromAssets and PipelineResources_AddModel."
  },
  {
    "title": "Monitor",
    "href": "api/lua/classes.html#monitor",
    "kind": "class",
    "text": ""
  },
  {
    "title": "MonitorList",
    "href": "api/lua/classes.html#monitorlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "MonitorMode",
    "href": "api/lua/classes.html#monitormode",
    "kind": "class",
    "text": ""
  },
  {
    "title": "MonitorModeList",
    "href": "api/lua/classes.html#monitormodelist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Mouse",
    "href": "api/lua/classes.html#mouse",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetMouseNames to query for available mouse devices."
  },
  {
    "title": "MouseState",
    "href": "api/lua/classes.html#mousestate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "NavMesh",
    "href": "api/lua/classes.html#navmesh",
    "kind": "class",
    "text": "Navigation mesh that can be queried using a NavMeshQuery to compute the most efficient path between two world-space positions."
  },
  {
    "title": "NavMeshQuery",
    "href": "api/lua/classes.html#navmeshquery",
    "kind": "class",
    "text": "Navigation mesh query object. Queries are performed in world space. See man.Navigation."
  },
  {
    "title": "Node",
    "href": "api/lua/classes.html#node",
    "kind": "class",
    "text": "The base element of a scene, see man.Scene."
  },
  {
    "title": "NodeList",
    "href": "api/lua/classes.html#nodelist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "NodePairContacts",
    "href": "api/lua/classes.html#nodepaircontacts",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Object",
    "href": "api/lua/classes.html#object",
    "kind": "class",
    "text": "This components draws a Model. It stores the material table used to draw the model."
  },
  {
    "title": "OpenVREye",
    "href": "api/lua/classes.html#openvreye",
    "kind": "class",
    "text": "Matrices for a VR eye, see OpenVRState."
  },
  {
    "title": "OpenVREyeFrameBuffer",
    "href": "api/lua/classes.html#openvreyeframebuffer",
    "kind": "class",
    "text": "Framebuffer for a VR eye. Render to two such buffer, one for each eye, before submitting them using OpenVRSubmitFrame."
  },
  {
    "title": "OpenVRState",
    "href": "api/lua/classes.html#openvrstate",
    "kind": "class",
    "text": "OpenVR state including the body and head transformations, the left and right eye states and the render target dimensions expected by the backend."
  },
  {
    "title": "OpenXREyeFrameBuffer",
    "href": "api/lua/classes.html#openxreyeframebuffer",
    "kind": "class",
    "text": ""
  },
  {
    "title": "OpenXREyeFrameBufferList",
    "href": "api/lua/classes.html#openxreyeframebufferlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "OpenXRFrameInfo",
    "href": "api/lua/classes.html#openxrframeinfo",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Picture",
    "href": "api/lua/classes.html#picture",
    "kind": "class",
    "text": "The picture origin (0, 0) is in the top-left corner of its frame with the X and Y axises increasing toward the right and bottom. To load and save a picture use LoadPicture, LoadPNG or SavePNG. The Picture_SetData and Picture_GetData methods"
  },
  {
    "title": "Pipeline",
    "href": "api/lua/classes.html#pipeline",
    "kind": "class",
    "text": "Rendering pipeline base class."
  },
  {
    "title": "PipelineInfo",
    "href": "api/lua/classes.html#pipelineinfo",
    "kind": "class",
    "text": ""
  },
  {
    "title": "PipelineProgram",
    "href": "api/lua/classes.html#pipelineprogram",
    "kind": "class",
    "text": ""
  },
  {
    "title": "PipelineProgramRef",
    "href": "api/lua/classes.html#pipelineprogramref",
    "kind": "class",
    "text": ""
  },
  {
    "title": "PipelineResources",
    "href": "api/lua/classes.html#pipelineresources",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ProfilerFrame",
    "href": "api/lua/classes.html#profilerframe",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ProgramHandle",
    "href": "api/lua/classes.html#programhandle",
    "kind": "class",
    "text": "Handle to a shader program."
  },
  {
    "title": "Quaternion",
    "href": "api/lua/classes.html#quaternion",
    "kind": "class",
    "text": "Quaternion can be used to represent a 3d rotation. It provides a more compact representation of the rotation than Mat3 and can efficiently and correctly interpolate (see Slerp) between two rotations."
  },
  {
    "title": "RaycastOut",
    "href": "api/lua/classes.html#raycastout",
    "kind": "class",
    "text": "Contains the result of a physics raycast. * `P`: Position of the raycast hit * `N`: Normal of the raycast hit * `Node`: Node hit by the raycast * `t`: Parametric value of the intersection, ratio of the distance to the hit by the length of t"
  },
  {
    "title": "RaycastOutList",
    "href": "api/lua/classes.html#raycastoutlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Rect",
    "href": "api/lua/classes.html#rect",
    "kind": "class",
    "text": ""
  },
  {
    "title": "RenderState",
    "href": "api/lua/classes.html#renderstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "RigidBody",
    "href": "api/lua/classes.html#rigidbody",
    "kind": "class",
    "text": "Rigid body component, see man.Physics."
  },
  {
    "title": "SAO",
    "href": "api/lua/classes.html#sao",
    "kind": "class",
    "text": "Ambient occlusion post-process object holding internal states and resources. Create with CreateSAOFromFile or CreateSAOFromAssets, use with ComputeSAO, finally call DestroySAO to dispose of resources when done."
  },
  {
    "title": "Scene",
    "href": "api/lua/classes.html#scene",
    "kind": "class",
    "text": "A scene object representing a world populated with Node, see man.Scene."
  },
  {
    "title": "SceneAnimRef",
    "href": "api/lua/classes.html#sceneanimref",
    "kind": "class",
    "text": "Reference to a scene animation."
  },
  {
    "title": "SceneAnimRefList",
    "href": "api/lua/classes.html#sceneanimreflist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SceneBullet3Physics",
    "href": "api/lua/classes.html#scenebullet3physics",
    "kind": "class",
    "text": "Newton physics for scene physics and collision components. See man.Physics."
  },
  {
    "title": "SceneBullet3PhysicsPreTickCallback",
    "href": "api/lua/classes.html#scenebullet3physicspretickcallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SceneClocks",
    "href": "api/lua/classes.html#sceneclocks",
    "kind": "class",
    "text": "Holds clocks for the different scene systems. This is required as some system such as the physics system may run at a different rate than the scene."
  },
  {
    "title": "SceneForwardPipelinePassViewId",
    "href": "api/lua/classes.html#sceneforwardpipelinepassviewid",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SceneForwardPipelineRenderData",
    "href": "api/lua/classes.html#sceneforwardpipelinerenderdata",
    "kind": "class",
    "text": "Holds all data required to draw a scene with the forward pipeline. See man.ForwardPipeline."
  },
  {
    "title": "SceneLuaVM",
    "href": "api/lua/classes.html#sceneluavm",
    "kind": "class",
    "text": "Lua VM for scene script components. See man.Scripting."
  },
  {
    "title": "ScenePlayAnimRef",
    "href": "api/lua/classes.html#sceneplayanimref",
    "kind": "class",
    "text": "Reference to a playing scene animation."
  },
  {
    "title": "ScenePlayAnimRefList",
    "href": "api/lua/classes.html#sceneplayanimreflist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SceneView",
    "href": "api/lua/classes.html#sceneview",
    "kind": "class",
    "text": "Holds a view to a subset of a scene. Used by the instance system to track instantiated scene content. See Node_GetInstanceSceneView and man.Scene."
  },
  {
    "title": "Script",
    "href": "api/lua/classes.html#script",
    "kind": "class",
    "text": ""
  },
  {
    "title": "ScriptList",
    "href": "api/lua/classes.html#scriptlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SetDrawStatesCallback",
    "href": "api/lua/classes.html#setdrawstatescallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Signal_returning_void_taking_const_char_ptr",
    "href": "api/lua/classes.html#signal_returning_void_taking_const_char_ptr",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Signal_returning_void_taking_time_ns",
    "href": "api/lua/classes.html#signal_returning_void_taking_time_ns",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SpatializedSourceState",
    "href": "api/lua/classes.html#spatializedsourcestate",
    "kind": "class",
    "text": "State for a spatialized audio source, see man.Audio."
  },
  {
    "title": "SRanipalEyeState",
    "href": "api/lua/classes.html#sranipaleyestate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "SRanipalState",
    "href": "api/lua/classes.html#sranipalstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "StereoSourceState",
    "href": "api/lua/classes.html#stereosourcestate",
    "kind": "class",
    "text": "State for a stereo audio source, see man.Audio."
  },
  {
    "title": "StringList",
    "href": "api/lua/classes.html#stringlist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TextInputCallback",
    "href": "api/lua/classes.html#textinputcallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TextInputCallbackConnection",
    "href": "api/lua/classes.html#textinputcallbackconnection",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Texture",
    "href": "api/lua/classes.html#texture",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TextureInfo",
    "href": "api/lua/classes.html#textureinfo",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TextureRef",
    "href": "api/lua/classes.html#textureref",
    "kind": "class",
    "text": ""
  },
  {
    "title": "TimeCallback",
    "href": "api/lua/classes.html#timecallback",
    "kind": "class",
    "text": "A function taking a time value as parameter with no return value, see man.CoordinateAndUnitSystem."
  },
  {
    "title": "TimeCallbackConnection",
    "href": "api/lua/classes.html#timecallbackconnection",
    "kind": "class",
    "text": "A TimeCallback connection to a Signal_returning_void_taking_time_ns."
  },
  {
    "title": "Transform",
    "href": "api/lua/classes.html#transform",
    "kind": "class",
    "text": "Transformation component for a Node, see man.Scene."
  },
  {
    "title": "TransformTRS",
    "href": "api/lua/classes.html#transformtrs",
    "kind": "class",
    "text": "Translation, rotation and scale packed as a single object."
  },
  {
    "title": "UInt16List",
    "href": "api/lua/classes.html#uint16list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "UInt32List",
    "href": "api/lua/classes.html#uint32list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "UniformSetTexture",
    "href": "api/lua/classes.html#uniformsettexture",
    "kind": "class",
    "text": "Command object to set a uniform texture at draw time."
  },
  {
    "title": "UniformSetTextureList",
    "href": "api/lua/classes.html#uniformsettexturelist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "UniformSetValue",
    "href": "api/lua/classes.html#uniformsetvalue",
    "kind": "class",
    "text": "Command object to set a uniform value at draw time."
  },
  {
    "title": "UniformSetValueList",
    "href": "api/lua/classes.html#uniformsetvaluelist",
    "kind": "class",
    "text": ""
  },
  {
    "title": "update_controllersCallback",
    "href": "api/lua/classes.html#update_controllerscallback",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Vec2",
    "href": "api/lua/classes.html#vec2",
    "kind": "class",
    "text": "2-dimensional floating point vector."
  },
  {
    "title": "Vec2List",
    "href": "api/lua/classes.html#vec2list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Vec3",
    "href": "api/lua/classes.html#vec3",
    "kind": "class",
    "text": "3-dimensional vector."
  },
  {
    "title": "Vec3List",
    "href": "api/lua/classes.html#vec3list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Vec4",
    "href": "api/lua/classes.html#vec4",
    "kind": "class",
    "text": "4-dimensional vector."
  },
  {
    "title": "Vec4List",
    "href": "api/lua/classes.html#vec4list",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Vertex",
    "href": "api/lua/classes.html#vertex",
    "kind": "class",
    "text": ""
  },
  {
    "title": "VertexLayout",
    "href": "api/lua/classes.html#vertexlayout",
    "kind": "class",
    "text": "Memory layout and types of vertex attributes."
  },
  {
    "title": "Vertices",
    "href": "api/lua/classes.html#vertices",
    "kind": "class",
    "text": "Helper class to generate vertex buffers for drawing primitives."
  },
  {
    "title": "ViewState",
    "href": "api/lua/classes.html#viewstate",
    "kind": "class",
    "text": "Everything required to define an observer inside a 3d world. This object holds the projection matrix and its associated frustum as well as the transformation of the observer. The world content is transformed by the observer view matrix befo"
  },
  {
    "title": "VoidPointer",
    "href": "api/lua/classes.html#voidpointer",
    "kind": "class",
    "text": ""
  },
  {
    "title": "VRController",
    "href": "api/lua/classes.html#vrcontroller",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetVRControllerNames to query for available VR controller devices."
  },
  {
    "title": "VRControllerState",
    "href": "api/lua/classes.html#vrcontrollerstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "VRGenericTracker",
    "href": "api/lua/classes.html#vrgenerictracker",
    "kind": "class",
    "text": "Helper class holding the current and previous device state to enable delta state queries. Use GetVRGenericTrackerNames to query for available VR generic tracker devices."
  },
  {
    "title": "VRGenericTrackerState",
    "href": "api/lua/classes.html#vrgenerictrackerstate",
    "kind": "class",
    "text": ""
  },
  {
    "title": "Window",
    "href": "api/lua/classes.html#window",
    "kind": "class",
    "text": "Window object."
  },
  {
    "title": "Abs",
    "href": "api/lua/functions.html#abs",
    "kind": "function",
    "text": "Return the absolute value of the function input. For vectors, the absolute value is applied to each component individually and the resulting vector is returned."
  },
  {
    "title": "AddAssetsFolder",
    "href": "api/lua/functions.html#addassetsfolder",
    "kind": "function",
    "text": "Mount a local filesystem folder as an assets source. See man.Assets."
  },
  {
    "title": "AddAssetsPackage",
    "href": "api/lua/functions.html#addassetspackage",
    "kind": "function",
    "text": "Mount an archive stored on the local filesystem as an assets source. See man.Assets."
  },
  {
    "title": "AlphaScale",
    "href": "api/lua/functions.html#alphascale",
    "kind": "function",
    "text": "Scale the alpha component of the input color."
  },
  {
    "title": "ApplyBloom",
    "href": "api/lua/functions.html#applybloom",
    "kind": "function",
    "text": "Process `input` texture and generate a bloom overlay on top of `output`, input and output must be of the same size. Use CreateBloomFromFile/CreateBloomFromAssets to create a Bloom object and DestroyBloom to destroy its internal resources af"
  },
  {
    "title": "ARGB32",
    "href": "api/lua/functions.html#argb32",
    "kind": "function",
    "text": "Create a 32 bit integer ARGB color."
  },
  {
    "title": "ARGB32ToRGBA32",
    "href": "api/lua/functions.html#argb32torgba32",
    "kind": "function",
    "text": "Convert a 32 bit integer ARGB color to RGBA."
  },
  {
    "title": "AudioInit",
    "href": "api/lua/functions.html#audioinit",
    "kind": "function",
    "text": "Initialize the audio system."
  },
  {
    "title": "AudioShutdown",
    "href": "api/lua/functions.html#audioshutdown",
    "kind": "function",
    "text": "Shutdown the audio system."
  },
  {
    "title": "BaseToEuler",
    "href": "api/lua/functions.html#basetoeuler",
    "kind": "function",
    "text": "Compute the Euler angles triplet for the provided `z` direction. The up-vector `y` can be provided to improve coherency of the returned values over time."
  },
  {
    "title": "BeginProfilerSection",
    "href": "api/lua/functions.html#beginprofilersection",
    "kind": "function",
    "text": "Begin a named profiler section. Call EndProfilerSection to end the section."
  },
  {
    "title": "CaptureProfilerFrame",
    "href": "api/lua/functions.html#captureprofilerframe",
    "kind": "function",
    "text": "Capture the current profiler frame but do not end it. See EndProfilerFrame to capture and end the current profiler frame. See PrintProfilerFrame to print a profiler frame to the console."
  },
  {
    "title": "CaptureTexture",
    "href": "api/lua/functions.html#capturetexture",
    "kind": "function",
    "text": "Capture a texture content to a Picture. Return the frame counter at which the capture will be complete. A Picture object can be accessed by the CPU. This function is asynchronous and its result will not be available until the returned frame"
  },
  {
    "title": "Cast_Pipeline_To_ForwardPipeline",
    "href": "api/lua/functions.html#cast_pipeline_to_forwardpipeline",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Ceil",
    "href": "api/lua/functions.html#ceil",
    "kind": "function",
    "text": "Returns a vector whose elements are equal to the nearest integer greater than or equal to the vector elements."
  },
  {
    "title": "ChromaScale",
    "href": "api/lua/functions.html#chromascale",
    "kind": "function",
    "text": "Return a copy of the color with its saturation scaled as specified."
  },
  {
    "title": "Clamp",
    "href": "api/lua/functions.html#clamp",
    "kind": "function",
    "text": "Return a vector whose elements are equal to the vector elements clipped to the specified interval."
  },
  {
    "title": "ClampLen",
    "href": "api/lua/functions.html#clamplen",
    "kind": "function",
    "text": "Returns a vector in the same direction as the specified vector, but with its length clipped by the specified interval."
  },
  {
    "title": "ClassifyLine",
    "href": "api/lua/functions.html#classifyline",
    "kind": "function",
    "text": "Return `true` if the provided line intersect the bounding volume, `false` otherwise."
  },
  {
    "title": "ClassifySegment",
    "href": "api/lua/functions.html#classifysegment",
    "kind": "function",
    "text": "Return `true` if the provided segment intersect the bounding volume, `false` otherwise."
  },
  {
    "title": "CleanPath",
    "href": "api/lua/functions.html#cleanpath",
    "kind": "function",
    "text": "Cleanup a local filesystem path according to the host platform conventions. - Remove redundant folder separators. - Remove redundant `.` and `..` folder entries. - Ensure forward slash (`/`) folder separators on Unix and back slash (`\\`) fo"
  },
  {
    "title": "ClipSpaceToScreenSpace",
    "href": "api/lua/functions.html#clipspacetoscreenspace",
    "kind": "function",
    "text": "Convert a 3d position in clip space (homogeneous space) to a 2d position on screen."
  },
  {
    "title": "Close",
    "href": "api/lua/functions.html#close",
    "kind": "function",
    "text": "Close a file handle."
  },
  {
    "title": "Cm",
    "href": "api/lua/functions.html#cm",
    "kind": "function",
    "text": "Convert a value in centimeters to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "ColorFromABGR32",
    "href": "api/lua/functions.html#colorfromabgr32",
    "kind": "function",
    "text": "Create a color from a 32 bit ABGR integer."
  },
  {
    "title": "ColorFromRGBA32",
    "href": "api/lua/functions.html#colorfromrgba32",
    "kind": "function",
    "text": "Create a color from a 32 bit RGBA integer."
  },
  {
    "title": "ColorFromVector3",
    "href": "api/lua/functions.html#colorfromvector3",
    "kind": "function",
    "text": "Create a color from a 3d vector, alpha defaults to 1."
  },
  {
    "title": "ColorFromVector4",
    "href": "api/lua/functions.html#colorfromvector4",
    "kind": "function",
    "text": "Return a 4-dimensional vector as a color."
  },
  {
    "title": "ColorI",
    "href": "api/lua/functions.html#colori",
    "kind": "function",
    "text": "Create a color from integer values in the [0;255] range."
  },
  {
    "title": "ColorToABGR32",
    "href": "api/lua/functions.html#colortoabgr32",
    "kind": "function",
    "text": "Return a 32 bit ABGR integer from a color."
  },
  {
    "title": "ColorToGrayscale",
    "href": "api/lua/functions.html#colortograyscale",
    "kind": "function",
    "text": "Return the grayscale representation of a color. A weighted average is used to account for human perception of colors."
  },
  {
    "title": "ColorToRGBA32",
    "href": "api/lua/functions.html#colortorgba32",
    "kind": "function",
    "text": "Return a 32 bit RGBA integer from a color."
  },
  {
    "title": "Compute2DProjectionMatrix",
    "href": "api/lua/functions.html#compute2dprojectionmatrix",
    "kind": "function",
    "text": "Returns a projection matrix from a 2D space to the 3D world, as required by SetViewTransform for example."
  },
  {
    "title": "ComputeAspectRatioX",
    "href": "api/lua/functions.html#computeaspectratiox",
    "kind": "function",
    "text": "Compute the aspect ratio factor for the provided viewport dimensions. Use this method to compute aspect ratio for landscape display. See ComputeAspectRatioY."
  },
  {
    "title": "ComputeAspectRatioY",
    "href": "api/lua/functions.html#computeaspectratioy",
    "kind": "function",
    "text": "Compute the aspect ratio factor for the provided viewport dimensions. Use this method to compute aspect ratio for portrait display. See ComputeAspectRatioX."
  },
  {
    "title": "ComputeMinMaxBoundingSphere",
    "href": "api/lua/functions.html#computeminmaxboundingsphere",
    "kind": "function",
    "text": "Compute the bounding sphere for the provided axis-aligned bounding box."
  },
  {
    "title": "ComputeOrthographicProjectionMatrix",
    "href": "api/lua/functions.html#computeorthographicprojectionmatrix",
    "kind": "function",
    "text": "Compute an orthographic projection matrix. An orthographic projection has no perspective and all lines parrallel in 3d space will still appear parrallel on screen after projection using the returned matrix. The `size` parameter controls the"
  },
  {
    "title": "ComputeOrthographicViewState",
    "href": "api/lua/functions.html#computeorthographicviewstate",
    "kind": "function",
    "text": "Compute an orthographic view state. The `size` parameter controls the extends of the projected view. When projecting a 3d world this parameter is expressed in meters. Use the `aspect_ratio` parameter to prevent distortion from induced by no"
  },
  {
    "title": "ComputePerspectiveProjectionMatrix",
    "href": "api/lua/functions.html#computeperspectiveprojectionmatrix",
    "kind": "function",
    "text": "Compute a perspective projection matrix, , `fov` is the field of view angle, see Deg and Rad. See ZoomFactorToFov, FovToZoomFactor, ComputeAspectRatioX and ComputeAspectRatioY."
  },
  {
    "title": "ComputePerspectiveViewState",
    "href": "api/lua/functions.html#computeperspectiveviewstate",
    "kind": "function",
    "text": "Compute a perspective view state. See ComputePerspectiveProjectionMatrix, ZoomFactorToFov, FovToZoomFactor, ComputeAspectRatioX and ComputeAspectRatioY."
  },
  {
    "title": "ComputeRenderState",
    "href": "api/lua/functions.html#computerenderstate",
    "kind": "function",
    "text": "Compute a render state to control subsequent render calls culling mode, blending mode, Z mask, etc... The same render state can be used by different render calls. See DrawLines, DrawTriangles and DrawModel."
  },
  {
    "title": "ComputeSAO",
    "href": "api/lua/functions.html#computesao",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ComputeSortKey",
    "href": "api/lua/functions.html#computesortkey",
    "kind": "function",
    "text": "Compute a sorting key to control the rendering order of a display list, `view_depth` is expected in view space."
  },
  {
    "title": "ComputeSortKeyFromWorld",
    "href": "api/lua/functions.html#computesortkeyfromworld",
    "kind": "function",
    "text": "Compute a sorting key to control the rendering order of a display list."
  },
  {
    "title": "ComputeTextHeight",
    "href": "api/lua/functions.html#computetextheight",
    "kind": "function",
    "text": "Compute the height of a text string."
  },
  {
    "title": "ComputeTextRect",
    "href": "api/lua/functions.html#computetextrect",
    "kind": "function",
    "text": "Compute the width and height of a text string."
  },
  {
    "title": "ConfigureCrowdAgent",
    "href": "api/lua/functions.html#configurecrowdagent",
    "kind": "function",
    "text": "Create a parameter structure for a crowd agent to be used with a Crowd object."
  },
  {
    "title": "Contains",
    "href": "api/lua/functions.html#contains",
    "kind": "function",
    "text": "Return `true` if the provided position is inside the bounding volume, `false` otherwise."
  },
  {
    "title": "CopyDir",
    "href": "api/lua/functions.html#copydir",
    "kind": "function",
    "text": "Copy a directory on the local filesystem, this function does not recurse through subdirectories. See CopyDirRecursive."
  },
  {
    "title": "CopyDirRecursive",
    "href": "api/lua/functions.html#copydirrecursive",
    "kind": "function",
    "text": "Copy a directory on the local filesystem, recurse through subdirectories."
  },
  {
    "title": "CopyFile",
    "href": "api/lua/functions.html#copyfile",
    "kind": "function",
    "text": "Copy a file on the local filesystem."
  },
  {
    "title": "CosineInterpolate",
    "href": "api/lua/functions.html#cosineinterpolate",
    "kind": "function",
    "text": "Compute the cosine interpolated value between `y0` and `y1` at `t`. See LinearInterpolate, CubicInterpolate and HermiteInterpolate."
  },
  {
    "title": "CreateBloomFromAssets",
    "href": "api/lua/functions.html#createbloomfromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateBloomFromFile",
    "href": "api/lua/functions.html#createbloomfromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateCamera",
    "href": "api/lua/functions.html#createcamera",
    "kind": "function",
    "text": "Create a new Node with a Transform and Camera components."
  },
  {
    "title": "CreateCapsuleModel",
    "href": "api/lua/functions.html#createcapsulemodel",
    "kind": "function",
    "text": "Create a capsule render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateConeModel",
    "href": "api/lua/functions.html#createconemodel",
    "kind": "function",
    "text": "Create a cone render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateCubeModel",
    "href": "api/lua/functions.html#createcubemodel",
    "kind": "function",
    "text": "Create a cube render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateCylinderModel",
    "href": "api/lua/functions.html#createcylindermodel",
    "kind": "function",
    "text": "Create a cylinder render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateForwardPipeline",
    "href": "api/lua/functions.html#createforwardpipeline",
    "kind": "function",
    "text": "Create a forward pipeline and its resources. See DestroyForwardPipeline."
  },
  {
    "title": "CreateForwardPipelineAAAFromAssets",
    "href": "api/lua/functions.html#createforwardpipelineaaafromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateForwardPipelineAAAFromFile",
    "href": "api/lua/functions.html#createforwardpipelineaaafromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateFrameBuffer",
    "href": "api/lua/functions.html#createframebuffer",
    "kind": "function",
    "text": "Create a framebuffer and its texture attachments. See DestroyFrameBuffer."
  },
  {
    "title": "CreateInstanceFromAssets",
    "href": "api/lua/functions.html#createinstancefromassets",
    "kind": "function",
    "text": "Helper function to create a Node with a Transform and an Instance component. The instance component will be setup and its resources loaded from the assets system. See man.Assets."
  },
  {
    "title": "CreateInstanceFromFile",
    "href": "api/lua/functions.html#createinstancefromfile",
    "kind": "function",
    "text": "Helper function to create a Node with a Transform and an Instance component. The instance component will be setup and its resources loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "CreateLinearLight",
    "href": "api/lua/functions.html#createlinearlight",
    "kind": "function",
    "text": "Helper function to create a Node with a Transform and a Light component."
  },
  {
    "title": "CreateMaterial",
    "href": "api/lua/functions.html#creatematerial",
    "kind": "function",
    "text": "Helper function to create a material. See SetMaterialProgram, SetMaterialValue and SetMaterialTexture."
  },
  {
    "title": "CreateMissingMaterialProgramValuesFromAssets",
    "href": "api/lua/functions.html#createmissingmaterialprogramvaluesfromassets",
    "kind": "function",
    "text": "This function scans the material program uniforms and creates a corresponding entry in the material if missing. Resources are loaded from the asset system if a default uniform value requires it. See man.Assets."
  },
  {
    "title": "CreateMissingMaterialProgramValuesFromFile",
    "href": "api/lua/functions.html#createmissingmaterialprogramvaluesfromfile",
    "kind": "function",
    "text": "This function scans the material program uniforms and creates a corresponding entry in the material if missing. Resources are loaded from the local filesystem if a default uniform value requires it."
  },
  {
    "title": "CreateNavMeshQuery",
    "href": "api/lua/functions.html#createnavmeshquery",
    "kind": "function",
    "text": "Create a navigation mesh query from a navigation mesh. See FindNavigationPathTo to perform an actual query."
  },
  {
    "title": "CreateObject",
    "href": "api/lua/functions.html#createobject",
    "kind": "function",
    "text": "Create a Node with a Transform and Object components."
  },
  {
    "title": "CreateOrthographicCamera",
    "href": "api/lua/functions.html#createorthographiccamera",
    "kind": "function",
    "text": "Create a Node with a Transform and a Camera component."
  },
  {
    "title": "CreatePhysicCube",
    "href": "api/lua/functions.html#createphysiccube",
    "kind": "function",
    "text": "Create a Node with a Transform, Object and RigidBody components."
  },
  {
    "title": "CreatePhysicSphere",
    "href": "api/lua/functions.html#createphysicsphere",
    "kind": "function",
    "text": "Create a Node with a Transform, Object and RigidBody components."
  },
  {
    "title": "CreatePlaneModel",
    "href": "api/lua/functions.html#createplanemodel",
    "kind": "function",
    "text": "Create a plane render model."
  },
  {
    "title": "CreatePointLight",
    "href": "api/lua/functions.html#createpointlight",
    "kind": "function",
    "text": "Create a Node with a Transform and a Light component."
  },
  {
    "title": "CreateSAOFromAssets",
    "href": "api/lua/functions.html#createsaofromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateSAOFromFile",
    "href": "api/lua/functions.html#createsaofromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "CreateSceneRootNode",
    "href": "api/lua/functions.html#createscenerootnode",
    "kind": "function",
    "text": "Helper function to create a Node with a Transform component then parent all root nodes in the scene to it."
  },
  {
    "title": "CreateScript",
    "href": "api/lua/functions.html#createscript",
    "kind": "function",
    "text": "Helper function to create a Node with a Script component."
  },
  {
    "title": "CreateSphereModel",
    "href": "api/lua/functions.html#createspheremodel",
    "kind": "function",
    "text": "Create a sphere render model. See CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel."
  },
  {
    "title": "CreateSpotLight",
    "href": "api/lua/functions.html#createspotlight",
    "kind": "function",
    "text": "Create a Node with a Transform and a Light component."
  },
  {
    "title": "CreateTexture",
    "href": "api/lua/functions.html#createtexture",
    "kind": "function",
    "text": "Create an empty texture. See CreateTextureFromPicture and UpdateTextureFromPicture."
  },
  {
    "title": "CreateTextureFromPicture",
    "href": "api/lua/functions.html#createtexturefrompicture",
    "kind": "function",
    "text": "Create a texture from a picture. See Picture, CreateTexture and UpdateTextureFromPicture."
  },
  {
    "title": "Crop",
    "href": "api/lua/functions.html#crop",
    "kind": "function",
    "text": "Crop a rectangle. Remove the specified amount of units on each side of the rectangle. See Grow."
  },
  {
    "title": "Cross",
    "href": "api/lua/functions.html#cross",
    "kind": "function",
    "text": "Return the cross product of two vectors."
  },
  {
    "title": "CrossProductMat3",
    "href": "api/lua/functions.html#crossproductmat3",
    "kind": "function",
    "text": "Creates a matrix __M__ so that __Mv = p⨯v__. Simply put, multiplying this matrix to any vector __v__ is equivalent to compute the cross product between __p__ and __v__."
  },
  {
    "title": "CubicInterpolate",
    "href": "api/lua/functions.html#cubicinterpolate",
    "kind": "function",
    "text": "Perform a cubic interpolation across four values with `t` in the [0;1] range between `y1` and `y2`. See LinearInterpolate, CosineInterpolate and HermiteInterpolate."
  },
  {
    "title": "CutFileExtension",
    "href": "api/lua/functions.html#cutfileextension",
    "kind": "function",
    "text": "Return a file path with its extension stripped. See CutFilePath and CutFileName."
  },
  {
    "title": "CutFileName",
    "href": "api/lua/functions.html#cutfilename",
    "kind": "function",
    "text": "Return the name part of a file path. All folder navigation and extension are stripped. See CutFileExtension and CutFilePath."
  },
  {
    "title": "CutFilePath",
    "href": "api/lua/functions.html#cutfilepath",
    "kind": "function",
    "text": "Return the folder navigation part of a file path. The file name and its extension are stripped. See CutFileExtension and CutFileName."
  },
  {
    "title": "Debug",
    "href": "api/lua/functions.html#debug",
    "kind": "function",
    "text": ""
  },
  {
    "title": "DebugSceneExplorer",
    "href": "api/lua/functions.html#debugsceneexplorer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Decompose",
    "href": "api/lua/functions.html#decompose",
    "kind": "function",
    "text": "Decompose a transformation matrix into its translation, scaling and rotation components."
  },
  {
    "title": "Deg",
    "href": "api/lua/functions.html#deg",
    "kind": "function",
    "text": "Convert an angle in degrees to the engine unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Deg3",
    "href": "api/lua/functions.html#deg3",
    "kind": "function",
    "text": "Convert a triplet of angles in degrees to the engine unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "DegreeToRadian",
    "href": "api/lua/functions.html#degreetoradian",
    "kind": "function",
    "text": "Convert an angle in degrees to radians."
  },
  {
    "title": "DestroyBloom",
    "href": "api/lua/functions.html#destroybloom",
    "kind": "function",
    "text": "Destroy a bloom post process object and all associated resources."
  },
  {
    "title": "DestroyForwardPipeline",
    "href": "api/lua/functions.html#destroyforwardpipeline",
    "kind": "function",
    "text": "Destroy a forward pipeline object."
  },
  {
    "title": "DestroyForwardPipelineAAA",
    "href": "api/lua/functions.html#destroyforwardpipelineaaa",
    "kind": "function",
    "text": ""
  },
  {
    "title": "DestroyFrameBuffer",
    "href": "api/lua/functions.html#destroyframebuffer",
    "kind": "function",
    "text": "Destroy a frame buffer and its resources."
  },
  {
    "title": "DestroyNavMesh",
    "href": "api/lua/functions.html#destroynavmesh",
    "kind": "function",
    "text": "Destroy a navigation mesh object."
  },
  {
    "title": "DestroyNavMeshQuery",
    "href": "api/lua/functions.html#destroynavmeshquery",
    "kind": "function",
    "text": "Destroy a navigation mesh query object."
  },
  {
    "title": "DestroyProgram",
    "href": "api/lua/functions.html#destroyprogram",
    "kind": "function",
    "text": "Destroy a shader program."
  },
  {
    "title": "DestroySAO",
    "href": "api/lua/functions.html#destroysao",
    "kind": "function",
    "text": "Destroy an ambient occlusion post process object and its resources."
  },
  {
    "title": "DestroyTexture",
    "href": "api/lua/functions.html#destroytexture",
    "kind": "function",
    "text": "Destroy a texture object."
  },
  {
    "title": "DestroyWindow",
    "href": "api/lua/functions.html#destroywindow",
    "kind": "function",
    "text": "Destroy a window object."
  },
  {
    "title": "Det",
    "href": "api/lua/functions.html#det",
    "kind": "function",
    "text": "Return the determinant of a matrix."
  },
  {
    "title": "DisableCursor",
    "href": "api/lua/functions.html#disablecursor",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Dist",
    "href": "api/lua/functions.html#dist",
    "kind": "function",
    "text": "Return the Euclidean distance between two vectors."
  },
  {
    "title": "Dist2",
    "href": "api/lua/functions.html#dist2",
    "kind": "function",
    "text": "Return the squared Euclidean distance between two vectors."
  },
  {
    "title": "DistanceToPlane",
    "href": "api/lua/functions.html#distancetoplane",
    "kind": "function",
    "text": "Return the signed distance from point __p__ to a plane. - Distance is positive if __p__ is in front of the plane, meaning that the plane normal is pointing towards __p__. - Distance is negative if __p__ is behind the plane, meaning that the"
  },
  {
    "title": "Dot",
    "href": "api/lua/functions.html#dot",
    "kind": "function",
    "text": "Return the dot product of two vectors."
  },
  {
    "title": "DrawLines",
    "href": "api/lua/functions.html#drawlines",
    "kind": "function",
    "text": "Draw a list of lines to the specified view. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program."
  },
  {
    "title": "DrawModel",
    "href": "api/lua/functions.html#drawmodel",
    "kind": "function",
    "text": "Draw a model to the specified view. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program."
  },
  {
    "title": "DrawNavMesh",
    "href": "api/lua/functions.html#drawnavmesh",
    "kind": "function",
    "text": "Draw a navigation mesh to the specified view. This is function is for debugging purpose. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program."
  },
  {
    "title": "DrawSprites",
    "href": "api/lua/functions.html#drawsprites",
    "kind": "function",
    "text": "Draw a list of sprites to the specified view. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program. *Note:* This function prepares the sprite on the CPU before submitting them all to the GPU as a si"
  },
  {
    "title": "DrawText",
    "href": "api/lua/functions.html#drawtext",
    "kind": "function",
    "text": "Write text to the specified view using the provided shader program and uniform values."
  },
  {
    "title": "DrawTriangles",
    "href": "api/lua/functions.html#drawtriangles",
    "kind": "function",
    "text": "Draw a list of triangles to the specified view. Use UniformSetValueList and UniformSetTextureList to pass uniform values to the shader program."
  },
  {
    "title": "DuplicateNodeAndChildrenFromAssets",
    "href": "api/lua/functions.html#duplicatenodeandchildrenfromassets",
    "kind": "function",
    "text": "Duplicate a node and its child hierarchy. Resources will be loaded from the assets system. See man.Assets."
  },
  {
    "title": "DuplicateNodeAndChildrenFromFile",
    "href": "api/lua/functions.html#duplicatenodeandchildrenfromfile",
    "kind": "function",
    "text": "Duplicate a node and its child hierarchy. Resources will be loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "DuplicateNodeFromAssets",
    "href": "api/lua/functions.html#duplicatenodefromassets",
    "kind": "function",
    "text": "Duplicate a node. Resources will be loaded from the assets system. See man.Assets."
  },
  {
    "title": "DuplicateNodeFromFile",
    "href": "api/lua/functions.html#duplicatenodefromfile",
    "kind": "function",
    "text": "Duplicate a node. Resources will be loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "DuplicateNodesAndChildrenFromAssets",
    "href": "api/lua/functions.html#duplicatenodesandchildrenfromassets",
    "kind": "function",
    "text": "Duplicate each node and children hierarchy of a list. Resources will be loaded from the assets system. See man.Assets."
  },
  {
    "title": "DuplicateNodesAndChildrenFromFile",
    "href": "api/lua/functions.html#duplicatenodesandchildrenfromfile",
    "kind": "function",
    "text": "Duplicate each node and children hierarchy of a list. Resources will be loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "DuplicateNodesFromAssets",
    "href": "api/lua/functions.html#duplicatenodesfromassets",
    "kind": "function",
    "text": "Duplicate each node of a list. Resources will be loaded from the assets system."
  },
  {
    "title": "DuplicateNodesFromFile",
    "href": "api/lua/functions.html#duplicatenodesfromfile",
    "kind": "function",
    "text": "Duplicate each node of a list. Resources will be loaded from the local filesystem. See man.Assets."
  },
  {
    "title": "EndProfilerFrame",
    "href": "api/lua/functions.html#endprofilerframe",
    "kind": "function",
    "text": "End a profiler frame and return it. See PrintProfilerFrame to print a profiler frame to the console."
  },
  {
    "title": "EndProfilerSection",
    "href": "api/lua/functions.html#endprofilersection",
    "kind": "function",
    "text": "End a named profiler section. Call BeginProfilerSection to begin a new section."
  },
  {
    "title": "Error",
    "href": "api/lua/functions.html#error",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Exists",
    "href": "api/lua/functions.html#exists",
    "kind": "function",
    "text": "Return `true` if a file exists on the local filesystem, `false` otherwise."
  },
  {
    "title": "ExtractZoomFactorFromProjectionMatrix",
    "href": "api/lua/functions.html#extractzoomfactorfromprojectionmatrix",
    "kind": "function",
    "text": "Extract zoom factor from a projection matrix. See ZoomFactorToFov."
  },
  {
    "title": "ExtractZRangeFromOrthographicProjectionMatrix",
    "href": "api/lua/functions.html#extractzrangefromorthographicprojectionmatrix",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ExtractZRangeFromPerspectiveProjectionMatrix",
    "href": "api/lua/functions.html#extractzrangefromperspectiveprojectionmatrix",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ExtractZRangeFromProjectionMatrix",
    "href": "api/lua/functions.html#extractzrangefromprojectionmatrix",
    "kind": "function",
    "text": "Extract z near and z far clipping range from a projection matrix."
  },
  {
    "title": "FaceForward",
    "href": "api/lua/functions.html#faceforward",
    "kind": "function",
    "text": "Return the provided vector facing toward the provided direction. If the angle between `v` and `d` is less than 90° then `v` is returned unchanged, `v` will be returned reversed otherwise."
  },
  {
    "title": "FactorizePath",
    "href": "api/lua/functions.html#factorizepath",
    "kind": "function",
    "text": "Return the input path with all redundant navigation entries stripped (folder separator, `..` and `.` entries)."
  },
  {
    "title": "FileToString",
    "href": "api/lua/functions.html#filetostring",
    "kind": "function",
    "text": "Return the content of a local filesystem as a string."
  },
  {
    "title": "FindNavigationPathTo",
    "href": "api/lua/functions.html#findnavigationpathto",
    "kind": "function",
    "text": "Return the navigation path between `from` and `to` as a list of Vec3 world positions. See CreateNavMeshQuery."
  },
  {
    "title": "FitsInside",
    "href": "api/lua/functions.html#fitsinside",
    "kind": "function",
    "text": "Return wether `a` fits in `b`."
  },
  {
    "title": "Floor",
    "href": "api/lua/functions.html#floor",
    "kind": "function",
    "text": "Returns a vector whose elements are equal to the nearest integer less than or equal to the vector elements."
  },
  {
    "title": "FovToZoomFactor",
    "href": "api/lua/functions.html#fovtozoomfactor",
    "kind": "function",
    "text": "Convert from a fov value in radian to a zoom factor value in meters."
  },
  {
    "title": "FpsController",
    "href": "api/lua/functions.html#fpscontroller",
    "kind": "function",
    "text": "Implement a first-person-shooter like controller. The input position and rotation parameters are returned modified according to the state of the control keys. This function is usually used by passing the current camera position and rotation"
  },
  {
    "title": "Frame",
    "href": "api/lua/functions.html#frame",
    "kind": "function",
    "text": "Advance the rendering backend to the next frame, execute all queued rendering commands. This function returns the backend current frame. The frame counter is used by asynchronous functions such as CaptureTexture. You must wait for the frame"
  },
  {
    "title": "FRand",
    "href": "api/lua/functions.html#frand",
    "kind": "function",
    "text": "Return a random floating point value in the provided range, default range is [0;1]. See Rand to generate a random integer value."
  },
  {
    "title": "FromHLS",
    "href": "api/lua/functions.html#fromhls",
    "kind": "function",
    "text": "Convert input hue/luminance/saturation color to RGBA, alpha channel is left unmodified."
  },
  {
    "title": "FRRand",
    "href": "api/lua/functions.html#frrand",
    "kind": "function",
    "text": "Return a random floating point value in the provided range, default range is [-1;1]."
  },
  {
    "title": "GaussianBlurIsoSurface",
    "href": "api/lua/functions.html#gaussianblurisosurface",
    "kind": "function",
    "text": "Apply a Gaussian blur to an iso-surface."
  },
  {
    "title": "GetArea",
    "href": "api/lua/functions.html#getarea",
    "kind": "function",
    "text": "Return the area of the volume."
  },
  {
    "title": "GetCenter",
    "href": "api/lua/functions.html#getcenter",
    "kind": "function",
    "text": "Return the center position of the volume."
  },
  {
    "title": "GetClock",
    "href": "api/lua/functions.html#getclock",
    "kind": "function",
    "text": "Return the current clock since the last call to TickClock or ResetClock. See time_to_sec_f to convert the returned time to second."
  },
  {
    "title": "GetClockDt",
    "href": "api/lua/functions.html#getclockdt",
    "kind": "function",
    "text": "Return the elapsed time recorded during the last call to TickClock."
  },
  {
    "title": "GetColorTexture",
    "href": "api/lua/functions.html#getcolortexture",
    "kind": "function",
    "text": "Retrieves color texture attachment."
  },
  {
    "title": "GetColumn",
    "href": "api/lua/functions.html#getcolumn",
    "kind": "function",
    "text": "Returns the nth column."
  },
  {
    "title": "GetCurrentWorkingDirectory",
    "href": "api/lua/functions.html#getcurrentworkingdirectory",
    "kind": "function",
    "text": "Return the system current working directory."
  },
  {
    "title": "GetDepthTexture",
    "href": "api/lua/functions.html#getdepthtexture",
    "kind": "function",
    "text": "Retrieves depth texture attachment."
  },
  {
    "title": "GetFileExtension",
    "href": "api/lua/functions.html#getfileextension",
    "kind": "function",
    "text": "Return the extension part of a file path."
  },
  {
    "title": "GetFileName",
    "href": "api/lua/functions.html#getfilename",
    "kind": "function",
    "text": "Return the name part of a file path (including its extension)."
  },
  {
    "title": "GetFilePath",
    "href": "api/lua/functions.html#getfilepath",
    "kind": "function",
    "text": "Return the path part of a file path (excluding file name and extension)."
  },
  {
    "title": "GetForwardPipelineInfo",
    "href": "api/lua/functions.html#getforwardpipelineinfo",
    "kind": "function",
    "text": "Return the pipeline info object for the forward pipeline."
  },
  {
    "title": "GetGamepadNames",
    "href": "api/lua/functions.html#getgamepadnames",
    "kind": "function",
    "text": "Return a list of names for all supported gamepad devices on the system. See ReadGamepad."
  },
  {
    "title": "GetHandJointAngularVelocity",
    "href": "api/lua/functions.html#gethandjointangularvelocity",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetHandJointLinearVelocity",
    "href": "api/lua/functions.html#gethandjointlinearvelocity",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetHandJointPose",
    "href": "api/lua/functions.html#gethandjointpose",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetHandJointRadius",
    "href": "api/lua/functions.html#gethandjointradius",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetHeight",
    "href": "api/lua/functions.html#getheight",
    "kind": "function",
    "text": "Return the height of a rectangle."
  },
  {
    "title": "GetJoystickDeviceNames",
    "href": "api/lua/functions.html#getjoystickdevicenames",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetJoystickNames",
    "href": "api/lua/functions.html#getjoysticknames",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetJsonBool",
    "href": "api/lua/functions.html#getjsonbool",
    "kind": "function",
    "text": "Return the value of a boolean JSON key."
  },
  {
    "title": "GetJsonFloat",
    "href": "api/lua/functions.html#getjsonfloat",
    "kind": "function",
    "text": "Return the value of a float JSON key."
  },
  {
    "title": "GetJsonInt",
    "href": "api/lua/functions.html#getjsonint",
    "kind": "function",
    "text": "Return the value of an integer JSON key."
  },
  {
    "title": "GetJsonString",
    "href": "api/lua/functions.html#getjsonstring",
    "kind": "function",
    "text": "Return the value of a string JSON key."
  },
  {
    "title": "GetKeyboardNames",
    "href": "api/lua/functions.html#getkeyboardnames",
    "kind": "function",
    "text": "Return a list of names for all supported keyboard devices on the system. See ReadKeyboard."
  },
  {
    "title": "GetKeyName",
    "href": "api/lua/functions.html#getkeyname",
    "kind": "function",
    "text": "Return the name for a keyboard key."
  },
  {
    "title": "GetMaterialAlphaCut",
    "href": "api/lua/functions.html#getmaterialalphacut",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialAmbientUsesUV1",
    "href": "api/lua/functions.html#getmaterialambientusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialBlendMode",
    "href": "api/lua/functions.html#getmaterialblendmode",
    "kind": "function",
    "text": "Return a material blending mode."
  },
  {
    "title": "GetMaterialDepthTest",
    "href": "api/lua/functions.html#getmaterialdepthtest",
    "kind": "function",
    "text": "Return a material depth test function."
  },
  {
    "title": "GetMaterialDiffuseUsesUV1",
    "href": "api/lua/functions.html#getmaterialdiffuseusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialFaceCulling",
    "href": "api/lua/functions.html#getmaterialfaceculling",
    "kind": "function",
    "text": "Return a material culling mode."
  },
  {
    "title": "GetMaterialNormalMapInWorldSpace",
    "href": "api/lua/functions.html#getmaterialnormalmapinworldspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialSkinning",
    "href": "api/lua/functions.html#getmaterialskinning",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialSpecularUsesUV1",
    "href": "api/lua/functions.html#getmaterialspecularusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetMaterialTexture",
    "href": "api/lua/functions.html#getmaterialtexture",
    "kind": "function",
    "text": "Return the texture reference assigned to a material named uniform."
  },
  {
    "title": "GetMaterialTextures",
    "href": "api/lua/functions.html#getmaterialtextures",
    "kind": "function",
    "text": "Return the list of names of a material texture uniforms."
  },
  {
    "title": "GetMaterialValues",
    "href": "api/lua/functions.html#getmaterialvalues",
    "kind": "function",
    "text": "Return the list of names of a material value uniforms."
  },
  {
    "title": "GetMaterialWriteRGBA",
    "href": "api/lua/functions.html#getmaterialwritergba",
    "kind": "function",
    "text": "Return the material color mask."
  },
  {
    "title": "GetMaterialWriteZ",
    "href": "api/lua/functions.html#getmaterialwritez",
    "kind": "function",
    "text": "Return the material depth write mask."
  },
  {
    "title": "GetMonitorModes",
    "href": "api/lua/functions.html#getmonitormodes",
    "kind": "function",
    "text": "Return the list of supported monitor modes."
  },
  {
    "title": "GetMonitorName",
    "href": "api/lua/functions.html#getmonitorname",
    "kind": "function",
    "text": "Return the monitor name."
  },
  {
    "title": "GetMonitorRect",
    "href": "api/lua/functions.html#getmonitorrect",
    "kind": "function",
    "text": "Returns a rectangle going from the position, in screen coordinates, of the upper-left corner of the specified monitor to the position of the lower-right corner."
  },
  {
    "title": "GetMonitors",
    "href": "api/lua/functions.html#getmonitors",
    "kind": "function",
    "text": "Return a list of monitors connected to the system."
  },
  {
    "title": "GetMonitorSizeMM",
    "href": "api/lua/functions.html#getmonitorsizemm",
    "kind": "function",
    "text": "Returns the size, in millimetres, of the display area of the specified monitor."
  },
  {
    "title": "GetMouseNames",
    "href": "api/lua/functions.html#getmousenames",
    "kind": "function",
    "text": "Return a list of names for all supported mouse devices on the system. See ReadKeyboard."
  },
  {
    "title": "GetNodePairContacts",
    "href": "api/lua/functions.html#getnodepaircontacts",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetNodesInContact",
    "href": "api/lua/functions.html#getnodesincontact",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetR",
    "href": "api/lua/functions.html#getr",
    "kind": "function",
    "text": "See GetRotation."
  },
  {
    "title": "GetRMatrix",
    "href": "api/lua/functions.html#getrmatrix",
    "kind": "function",
    "text": "See GetRotationMatrix."
  },
  {
    "title": "GetRotation",
    "href": "api/lua/functions.html#getrotation",
    "kind": "function",
    "text": "Return the rotation component of a transformation matrix as a Euler triplet."
  },
  {
    "title": "GetRotationMatrix",
    "href": "api/lua/functions.html#getrotationmatrix",
    "kind": "function",
    "text": "Return the rotation component of a transformation matrix as a Mat3 rotation matrix."
  },
  {
    "title": "GetRow",
    "href": "api/lua/functions.html#getrow",
    "kind": "function",
    "text": "Returns the nth row of a matrix."
  },
  {
    "title": "GetS",
    "href": "api/lua/functions.html#gets",
    "kind": "function",
    "text": "See GetScale."
  },
  {
    "title": "GetScale",
    "href": "api/lua/functions.html#getscale",
    "kind": "function",
    "text": "Return the scale component of a matrix a scale vector."
  },
  {
    "title": "GetSceneForwardPipelineFog",
    "href": "api/lua/functions.html#getsceneforwardpipelinefog",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetSceneForwardPipelineLights",
    "href": "api/lua/functions.html#getsceneforwardpipelinelights",
    "kind": "function",
    "text": "Filter through the scene lights and return a list of pipeline lights to be used by the scene forward pipeline."
  },
  {
    "title": "GetSceneForwardPipelinePassViewId",
    "href": "api/lua/functions.html#getsceneforwardpipelinepassviewid",
    "kind": "function",
    "text": "Return the view id for a scene forward pipeline pass id."
  },
  {
    "title": "GetSize",
    "href": "api/lua/functions.html#getsize",
    "kind": "function",
    "text": "Return the size in bytes of a local file."
  },
  {
    "title": "GetSourceDuration",
    "href": "api/lua/functions.html#getsourceduration",
    "kind": "function",
    "text": "Return the duration of an audio source."
  },
  {
    "title": "GetSourceState",
    "href": "api/lua/functions.html#getsourcestate",
    "kind": "function",
    "text": "Return the state of an audio source."
  },
  {
    "title": "GetSourceTimecode",
    "href": "api/lua/functions.html#getsourcetimecode",
    "kind": "function",
    "text": "Return the current timecode of a playing audio source."
  },
  {
    "title": "GetT",
    "href": "api/lua/functions.html#gett",
    "kind": "function",
    "text": "See GetTranslation."
  },
  {
    "title": "GetTextures",
    "href": "api/lua/functions.html#gettextures",
    "kind": "function",
    "text": "Returns color and depth texture attachments."
  },
  {
    "title": "GetTranslation",
    "href": "api/lua/functions.html#gettranslation",
    "kind": "function",
    "text": "Return the translation part of a tranformation matrix as a translation vector."
  },
  {
    "title": "GetUserFolder",
    "href": "api/lua/functions.html#getuserfolder",
    "kind": "function",
    "text": "Return the system user folder for the current user."
  },
  {
    "title": "GetVRControllerNames",
    "href": "api/lua/functions.html#getvrcontrollernames",
    "kind": "function",
    "text": "Return a list of names for all supported VR controller devices on the system. See ReadVRController."
  },
  {
    "title": "GetVRGenericTrackerNames",
    "href": "api/lua/functions.html#getvrgenerictrackernames",
    "kind": "function",
    "text": "Return a list of names for all supported VR tracker devices on the system."
  },
  {
    "title": "GetWidth",
    "href": "api/lua/functions.html#getwidth",
    "kind": "function",
    "text": "Return the width of a rectangle."
  },
  {
    "title": "GetWindowClientSize",
    "href": "api/lua/functions.html#getwindowclientsize",
    "kind": "function",
    "text": "Return a window client rectangle. The client area of a window does not include its decorations."
  },
  {
    "title": "GetWindowContentScale",
    "href": "api/lua/functions.html#getwindowcontentscale",
    "kind": "function",
    "text": ""
  },
  {
    "title": "GetWindowHandle",
    "href": "api/lua/functions.html#getwindowhandle",
    "kind": "function",
    "text": "Return the system native window handle."
  },
  {
    "title": "GetWindowInFocus",
    "href": "api/lua/functions.html#getwindowinfocus",
    "kind": "function",
    "text": "Return the system window with input focus."
  },
  {
    "title": "GetWindowPos",
    "href": "api/lua/functions.html#getwindowpos",
    "kind": "function",
    "text": "Return a window position on screen."
  },
  {
    "title": "GetWindowTitle",
    "href": "api/lua/functions.html#getwindowtitle",
    "kind": "function",
    "text": "Return a window title."
  },
  {
    "title": "GetX",
    "href": "api/lua/functions.html#getx",
    "kind": "function",
    "text": "Return the scaled X axis of a transformation matrix."
  },
  {
    "title": "GetY",
    "href": "api/lua/functions.html#gety",
    "kind": "function",
    "text": "Return the scaled Y axis of a transformation matrix."
  },
  {
    "title": "GetZ",
    "href": "api/lua/functions.html#getz",
    "kind": "function",
    "text": "Return the scaled Z axis of a transformation matrix."
  },
  {
    "title": "Grow",
    "href": "api/lua/functions.html#grow",
    "kind": "function",
    "text": "Grow a rectangle by the specified amount of units. See Crop."
  },
  {
    "title": "HasFileExtension",
    "href": "api/lua/functions.html#hasfileextension",
    "kind": "function",
    "text": "Test the extension of a file path."
  },
  {
    "title": "HermiteInterpolate",
    "href": "api/lua/functions.html#hermiteinterpolate",
    "kind": "function",
    "text": "Perform a Hermite interpolation across four values with `t` in the [0;1] range between `y1` and `y2`. The `tension` and `bias` parameters can be used to control the shape of underlying interpolation curve. See LinearInterpolate, CosineInter"
  },
  {
    "title": "HideCursor",
    "href": "api/lua/functions.html#hidecursor",
    "kind": "function",
    "text": "Hide the system mouse cursor. See ShowCursor."
  },
  {
    "title": "ImGuiAlignTextToFramePadding",
    "href": "api/lua/functions.html#imguialigntexttoframepadding",
    "kind": "function",
    "text": "Vertically align upcoming text baseline to FramePadding __y__ coordinate so that it will align properly to regularly framed items."
  },
  {
    "title": "ImGuiBegin",
    "href": "api/lua/functions.html#imguibegin",
    "kind": "function",
    "text": "Start a new window."
  },
  {
    "title": "ImGuiBeginChild",
    "href": "api/lua/functions.html#imguibeginchild",
    "kind": "function",
    "text": "Begin a scrolling region."
  },
  {
    "title": "ImGuiBeginCombo",
    "href": "api/lua/functions.html#imguibegincombo",
    "kind": "function",
    "text": "Begin a ImGui Combo Box."
  },
  {
    "title": "ImGuiBeginFrame",
    "href": "api/lua/functions.html#imguibeginframe",
    "kind": "function",
    "text": "Begin an ImGui frame. This function must be called once per frame before any other ImGui call. When using multiple contexts, it must be called for each context you intend to use during the current frame. See ImGuiEndFrame."
  },
  {
    "title": "ImGuiBeginGroup",
    "href": "api/lua/functions.html#imguibegingroup",
    "kind": "function",
    "text": "Lock horizontal starting position. Once closing a group it is seen as a single item (so you can use ImGuiIsItemHovered on a group, ImGuiSameLine between groups, etc...)."
  },
  {
    "title": "ImGuiBeginMainMenuBar",
    "href": "api/lua/functions.html#imguibeginmainmenubar",
    "kind": "function",
    "text": "Create and append to a full screen menu-bar. Note: Only call ImGuiEndMainMenuBar if this returns `true`."
  },
  {
    "title": "ImGuiBeginMenu",
    "href": "api/lua/functions.html#imguibeginmenu",
    "kind": "function",
    "text": "Create a sub-menu entry. Note: Only call ImGuiEndMenu if this returns `true`."
  },
  {
    "title": "ImGuiBeginMenuBar",
    "href": "api/lua/functions.html#imguibeginmenubar",
    "kind": "function",
    "text": "Start append to the menu-bar of the current window (requires the `WindowFlags_MenuBar` flag). Note: Only call ImGuiEndMenuBar if this returns `true`."
  },
  {
    "title": "ImGuiBeginPopup",
    "href": "api/lua/functions.html#imguibeginpopup",
    "kind": "function",
    "text": "Return `true` if popup is opened and starts outputting to it. Note: Only call ImGuiEndPopup if this returns `true`."
  },
  {
    "title": "ImGuiBeginPopupContextItem",
    "href": "api/lua/functions.html#imguibeginpopupcontextitem",
    "kind": "function",
    "text": "ImGui helper to open and begin popup when clicked on last item."
  },
  {
    "title": "ImGuiBeginPopupContextVoid",
    "href": "api/lua/functions.html#imguibeginpopupcontextvoid",
    "kind": "function",
    "text": "ImGui helper to open and begin popup when clicked in void (where there are no ImGui windows)"
  },
  {
    "title": "ImGuiBeginPopupContextWindow",
    "href": "api/lua/functions.html#imguibeginpopupcontextwindow",
    "kind": "function",
    "text": "ImGui helper to open and begin popup when clicked on current window."
  },
  {
    "title": "ImGuiBeginPopupModal",
    "href": "api/lua/functions.html#imguibeginpopupmodal",
    "kind": "function",
    "text": "Begin an ImGui modal dialog."
  },
  {
    "title": "ImGuiBeginTooltip",
    "href": "api/lua/functions.html#imguibegintooltip",
    "kind": "function",
    "text": "Used to create full-featured tooltip windows that aren't just text."
  },
  {
    "title": "ImGuiBullet",
    "href": "api/lua/functions.html#imguibullet",
    "kind": "function",
    "text": "Draw a small circle and keep the cursor on the same line. Advances by the same distance as an empty ImGuiTreeNode call."
  },
  {
    "title": "ImGuiBulletText",
    "href": "api/lua/functions.html#imguibullettext",
    "kind": "function",
    "text": "Draw a bullet followed by a static text."
  },
  {
    "title": "ImGuiButton",
    "href": "api/lua/functions.html#imguibutton",
    "kind": "function",
    "text": "Button widget returning `True` if the button was pressed."
  },
  {
    "title": "ImGuiCalcItemWidth",
    "href": "api/lua/functions.html#imguicalcitemwidth",
    "kind": "function",
    "text": "Returns the width of item given pushed settings and current cursor position. Note: This is not necessarily the width of last item."
  },
  {
    "title": "ImGuiCalcTextSize",
    "href": "api/lua/functions.html#imguicalctextsize",
    "kind": "function",
    "text": "Compute the bounding rectangle for the provided text."
  },
  {
    "title": "ImGuiCaptureKeyboardFromApp",
    "href": "api/lua/functions.html#imguicapturekeyboardfromapp",
    "kind": "function",
    "text": "Force capture keyboard when your widget is being hovered."
  },
  {
    "title": "ImGuiCaptureMouseFromApp",
    "href": "api/lua/functions.html#imguicapturemousefromapp",
    "kind": "function",
    "text": "Force capture mouse when your widget is being hovered."
  },
  {
    "title": "ImGuiCheckbox",
    "href": "api/lua/functions.html#imguicheckbox",
    "kind": "function",
    "text": "Display a checkbox widget. Returns an interaction flag (user interacted with the widget) and the current widget state (checked or not after user interaction). ```python was_clicked, my_value = gs.ImGuiCheckBox('My value', my_value) ```"
  },
  {
    "title": "ImGuiClearInputBuffer",
    "href": "api/lua/functions.html#imguiclearinputbuffer",
    "kind": "function",
    "text": "Force a reset of the ImGui input buffer."
  },
  {
    "title": "ImGuiCloseCurrentPopup",
    "href": "api/lua/functions.html#imguiclosecurrentpopup",
    "kind": "function",
    "text": "Close the popup we have begin-ed into. Clicking on a menu item or selectable automatically closes the current popup."
  },
  {
    "title": "ImGuiCollapsingHeader",
    "href": "api/lua/functions.html#imguicollapsingheader",
    "kind": "function",
    "text": "Draw a collapsing header, returns `False` if the header is collapsed so that you may skip drawing the header content."
  },
  {
    "title": "ImGuiColorButton",
    "href": "api/lua/functions.html#imguicolorbutton",
    "kind": "function",
    "text": "Color button widget, display a small colored rectangle."
  },
  {
    "title": "ImGuiColorEdit",
    "href": "api/lua/functions.html#imguicoloredit",
    "kind": "function",
    "text": "Color editor, returns the widget current color."
  },
  {
    "title": "ImGuiColumns",
    "href": "api/lua/functions.html#imguicolumns",
    "kind": "function",
    "text": "Begin a column layout section. To move to the next column use ImGuiNextColumn. To end a column layout section pass `1` to this function. **Note:** Current implementation supports a maximum of 64 columns."
  },
  {
    "title": "ImGuiCombo",
    "href": "api/lua/functions.html#imguicombo",
    "kind": "function",
    "text": "Combo box widget, return the current selection index. Combo items are passed as an array of string."
  },
  {
    "title": "ImGuiDragFloat",
    "href": "api/lua/functions.html#imguidragfloat",
    "kind": "function",
    "text": "Declare a widget to edit a float value. The widget can be dragged over to modify the underlying value."
  },
  {
    "title": "ImGuiDragIntVec2",
    "href": "api/lua/functions.html#imguidragintvec2",
    "kind": "function",
    "text": "Declare a widget to edit an iVec2 value. The widget can be dragged over to modify the underlying value."
  },
  {
    "title": "ImGuiDragVec2",
    "href": "api/lua/functions.html#imguidragvec2",
    "kind": "function",
    "text": "Declare a float edit widget that can be dragged over to modify its value."
  },
  {
    "title": "ImGuiDragVec3",
    "href": "api/lua/functions.html#imguidragvec3",
    "kind": "function",
    "text": "Declare a widget to edit a Vec3 value. The widget can be dragged over to modify the underlying value."
  },
  {
    "title": "ImGuiDragVec4",
    "href": "api/lua/functions.html#imguidragvec4",
    "kind": "function",
    "text": "Declare a widget to edit a Vec4 value. The widget can be dragged over to modify the underlying value."
  },
  {
    "title": "ImGuiDummy",
    "href": "api/lua/functions.html#imguidummy",
    "kind": "function",
    "text": "Add a dummy item of given size."
  },
  {
    "title": "ImGuiEnd",
    "href": "api/lua/functions.html#imguiend",
    "kind": "function",
    "text": "End the current window."
  },
  {
    "title": "ImGuiEndChild",
    "href": "api/lua/functions.html#imguiendchild",
    "kind": "function",
    "text": "End a scrolling region."
  },
  {
    "title": "ImGuiEndCombo",
    "href": "api/lua/functions.html#imguiendcombo",
    "kind": "function",
    "text": "End a combo widget."
  },
  {
    "title": "ImGuiEndFrame",
    "href": "api/lua/functions.html#imguiendframe",
    "kind": "function",
    "text": "End the current ImGui frame. All ImGui rendering is sent to the specified view. If no view is specified, view 255 is used. See man.Views."
  },
  {
    "title": "ImGuiEndGroup",
    "href": "api/lua/functions.html#imguiendgroup",
    "kind": "function",
    "text": "End the current group."
  },
  {
    "title": "ImGuiEndMainMenuBar",
    "href": "api/lua/functions.html#imguiendmainmenubar",
    "kind": "function",
    "text": "End the main menu bar. See ImGuiBeginMainMenuBar."
  },
  {
    "title": "ImGuiEndMenu",
    "href": "api/lua/functions.html#imguiendmenu",
    "kind": "function",
    "text": "End the current sub-menu entry."
  },
  {
    "title": "ImGuiEndMenuBar",
    "href": "api/lua/functions.html#imguiendmenubar",
    "kind": "function",
    "text": "End the current menu bar."
  },
  {
    "title": "ImGuiEndPopup",
    "href": "api/lua/functions.html#imguiendpopup",
    "kind": "function",
    "text": "End the current popup."
  },
  {
    "title": "ImGuiEndTooltip",
    "href": "api/lua/functions.html#imguiendtooltip",
    "kind": "function",
    "text": "End the current tooltip window. See ImGuiBeginTooltip."
  },
  {
    "title": "ImGuiGetColorU32",
    "href": "api/lua/functions.html#imguigetcoloru32",
    "kind": "function",
    "text": "Return a style color component as a 32 bit unsigned integer. See ImGuiPushStyleColor."
  },
  {
    "title": "ImGuiGetColumnIndex",
    "href": "api/lua/functions.html#imguigetcolumnindex",
    "kind": "function",
    "text": "Returns the index of the current column."
  },
  {
    "title": "ImGuiGetColumnOffset",
    "href": "api/lua/functions.html#imguigetcolumnoffset",
    "kind": "function",
    "text": "Returns the current column offset in pixels, from the left side of the content region."
  },
  {
    "title": "ImGuiGetColumnsCount",
    "href": "api/lua/functions.html#imguigetcolumnscount",
    "kind": "function",
    "text": "Return the number of columns in the current layout section. See ImGuiColumns."
  },
  {
    "title": "ImGuiGetColumnWidth",
    "href": "api/lua/functions.html#imguigetcolumnwidth",
    "kind": "function",
    "text": "Returns the current column width in pixels."
  },
  {
    "title": "ImGuiGetContentRegionAvail",
    "href": "api/lua/functions.html#imguigetcontentregionavail",
    "kind": "function",
    "text": "Get available space for content in the current layout."
  },
  {
    "title": "ImGuiGetContentRegionAvailWidth",
    "href": "api/lua/functions.html#imguigetcontentregionavailwidth",
    "kind": "function",
    "text": "Helper function to return the available width of current content region. See ImGuiGetContentRegionAvail."
  },
  {
    "title": "ImGuiGetContentRegionMax",
    "href": "api/lua/functions.html#imguigetcontentregionmax",
    "kind": "function",
    "text": "Return the available content space including window decorations and scrollbar."
  },
  {
    "title": "ImGuiGetCursorPos",
    "href": "api/lua/functions.html#imguigetcursorpos",
    "kind": "function",
    "text": "Return the layout cursor position in window space. Next widget declaration will take place at the cursor position. See ImGuiSetCursorPos and ImGuiSameLine."
  },
  {
    "title": "ImGuiGetCursorPosX",
    "href": "api/lua/functions.html#imguigetcursorposx",
    "kind": "function",
    "text": "Helper for ImGuiGetCursorPos."
  },
  {
    "title": "ImGuiGetCursorPosY",
    "href": "api/lua/functions.html#imguigetcursorposy",
    "kind": "function",
    "text": "Helper for ImGuiGetCursorPos."
  },
  {
    "title": "ImGuiGetCursorScreenPos",
    "href": "api/lua/functions.html#imguigetcursorscreenpos",
    "kind": "function",
    "text": "Return the current layout cursor position in screen space."
  },
  {
    "title": "ImGuiGetCursorStartPos",
    "href": "api/lua/functions.html#imguigetcursorstartpos",
    "kind": "function",
    "text": "Return the current layout \"line\" starting position. See ImGuiSameLine."
  },
  {
    "title": "ImGuiGetFont",
    "href": "api/lua/functions.html#imguigetfont",
    "kind": "function",
    "text": "Return the current ImGui font."
  },
  {
    "title": "ImGuiGetFontSize",
    "href": "api/lua/functions.html#imguigetfontsize",
    "kind": "function",
    "text": "Return the font size (height in pixels) of the current ImGui font with the current scale applied."
  },
  {
    "title": "ImGuiGetFontTexUvWhitePixel",
    "href": "api/lua/functions.html#imguigetfonttexuvwhitepixel",
    "kind": "function",
    "text": "Get UV coordinate for a while pixel, useful to draw custom shapes via the ImDrawList API."
  },
  {
    "title": "ImGuiGetFrameCount",
    "href": "api/lua/functions.html#imguigetframecount",
    "kind": "function",
    "text": "Return the ImGui frame counter. See ImGuiBeginFrame and ImGuiEndFrame."
  },
  {
    "title": "ImGuiGetFrameHeightWithSpacing",
    "href": "api/lua/functions.html#imguigetframeheightwithspacing",
    "kind": "function",
    "text": "Return the following value: FontSize + style.FramePadding.y * 2 + style.ItemSpacing.y (distance in pixels between 2 consecutive lines of framed widgets)"
  },
  {
    "title": "ImGuiGetID",
    "href": "api/lua/functions.html#imguigetid",
    "kind": "function",
    "text": "Return a unique ImGui ID."
  },
  {
    "title": "ImGuiGetItemRectMax",
    "href": "api/lua/functions.html#imguigetitemrectmax",
    "kind": "function",
    "text": "Get bounding rect maximum of last item in screen space."
  },
  {
    "title": "ImGuiGetItemRectMin",
    "href": "api/lua/functions.html#imguigetitemrectmin",
    "kind": "function",
    "text": "Get bounding rect minimum of last item in screen space."
  },
  {
    "title": "ImGuiGetItemRectSize",
    "href": "api/lua/functions.html#imguigetitemrectsize",
    "kind": "function",
    "text": "Get bounding rect size of last item in screen space."
  },
  {
    "title": "ImGuiGetMouseDragDelta",
    "href": "api/lua/functions.html#imguigetmousedragdelta",
    "kind": "function",
    "text": "Return the distance covered by the mouse cursor since the last button press."
  },
  {
    "title": "ImGuiGetMousePos",
    "href": "api/lua/functions.html#imguigetmousepos",
    "kind": "function",
    "text": "Return the mouse cursor coordinates in screen space."
  },
  {
    "title": "ImGuiGetMousePosOnOpeningCurrentPopup",
    "href": "api/lua/functions.html#imguigetmouseposonopeningcurrentpopup",
    "kind": "function",
    "text": "Retrieve a backup of the mouse position at the time of opening the current popup. See ImGuiBeginPopup."
  },
  {
    "title": "ImGuiGetScrollMaxX",
    "href": "api/lua/functions.html#imguigetscrollmaxx",
    "kind": "function",
    "text": "Get maximum scrolling amount on the horizontal axis."
  },
  {
    "title": "ImGuiGetScrollMaxY",
    "href": "api/lua/functions.html#imguigetscrollmaxy",
    "kind": "function",
    "text": "Get maximum scrolling amount on the vertical axis."
  },
  {
    "title": "ImGuiGetScrollX",
    "href": "api/lua/functions.html#imguigetscrollx",
    "kind": "function",
    "text": "Get scrolling amount on the horizontal axis."
  },
  {
    "title": "ImGuiGetScrollY",
    "href": "api/lua/functions.html#imguigetscrolly",
    "kind": "function",
    "text": "Get scrolling amount on the vertical axis."
  },
  {
    "title": "ImGuiGetTextLineHeight",
    "href": "api/lua/functions.html#imguigettextlineheight",
    "kind": "function",
    "text": "Return the height of a text line using the current font. See ImGuiPushFont."
  },
  {
    "title": "ImGuiGetTextLineHeightWithSpacing",
    "href": "api/lua/functions.html#imguigettextlineheightwithspacing",
    "kind": "function",
    "text": "Return the height of a text line using the current font plus vertical spacing between two layout lines. See ImGuiGetTextLineHeight."
  },
  {
    "title": "ImGuiGetTime",
    "href": "api/lua/functions.html#imguigettime",
    "kind": "function",
    "text": "Return the current ImGui time in seconds."
  },
  {
    "title": "ImGuiGetTreeNodeToLabelSpacing",
    "href": "api/lua/functions.html#imguigettreenodetolabelspacing",
    "kind": "function",
    "text": "Return the horizontal distance preceding label when using ImGuiTreeNode or ImGuiBullet. The value `g.FontSize + style.FramePadding.x * 2` is returned for a regular unframed TreeNode."
  },
  {
    "title": "ImGuiGetWindowContentRegionMax",
    "href": "api/lua/functions.html#imguigetwindowcontentregionmax",
    "kind": "function",
    "text": "Return the content boundaries max (roughly (0,0)+Size-Scroll) where Size can be override with ImGuiSetNextWindowContentSize, in window space."
  },
  {
    "title": "ImGuiGetWindowContentRegionMin",
    "href": "api/lua/functions.html#imguigetwindowcontentregionmin",
    "kind": "function",
    "text": "Content boundaries min (roughly (0,0)-Scroll), in window space."
  },
  {
    "title": "ImGuiGetWindowContentRegionWidth",
    "href": "api/lua/functions.html#imguigetwindowcontentregionwidth",
    "kind": "function",
    "text": "Return the width of the content region."
  },
  {
    "title": "ImGuiGetWindowDrawList",
    "href": "api/lua/functions.html#imguigetwindowdrawlist",
    "kind": "function",
    "text": "Get the draw list associated to the current window, to append your own drawing primitives."
  },
  {
    "title": "ImGuiGetWindowHeight",
    "href": "api/lua/functions.html#imguigetwindowheight",
    "kind": "function",
    "text": "Return the current window height."
  },
  {
    "title": "ImGuiGetWindowPos",
    "href": "api/lua/functions.html#imguigetwindowpos",
    "kind": "function",
    "text": "Return the current window position in screen space. See ImGuiSetWindowPos."
  },
  {
    "title": "ImGuiGetWindowSize",
    "href": "api/lua/functions.html#imguigetwindowsize",
    "kind": "function",
    "text": "Return the current window size. See ImGuiSetWindowSize."
  },
  {
    "title": "ImGuiGetWindowWidth",
    "href": "api/lua/functions.html#imguigetwindowwidth",
    "kind": "function",
    "text": "Return the current window width."
  },
  {
    "title": "ImGuiImage",
    "href": "api/lua/functions.html#imguiimage",
    "kind": "function",
    "text": "Display a texture as an image widget. See ImGuiImageButton."
  },
  {
    "title": "ImGuiImageButton",
    "href": "api/lua/functions.html#imguiimagebutton",
    "kind": "function",
    "text": "Declare an image button displaying the provided texture. See ImGuiImage."
  },
  {
    "title": "ImGuiIndent",
    "href": "api/lua/functions.html#imguiindent",
    "kind": "function",
    "text": "Move content position toward the right."
  },
  {
    "title": "ImGuiInit",
    "href": "api/lua/functions.html#imguiinit",
    "kind": "function",
    "text": "Initialize the global ImGui context. This function must be called once before any other ImGui function using the global context. See ImGuiInitContext."
  },
  {
    "title": "ImGuiInitContext",
    "href": "api/lua/functions.html#imguiinitcontext",
    "kind": "function",
    "text": "Initialize an ImGui context. This function must be called once before any other ImGui function using the context. See ImGuiInit."
  },
  {
    "title": "ImGuiInputFloat",
    "href": "api/lua/functions.html#imguiinputfloat",
    "kind": "function",
    "text": "Float field widget."
  },
  {
    "title": "ImGuiInputInt",
    "href": "api/lua/functions.html#imguiinputint",
    "kind": "function",
    "text": "Integer field widget."
  },
  {
    "title": "ImGuiInputIntVec2",
    "href": "api/lua/functions.html#imguiinputintvec2",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiInputText",
    "href": "api/lua/functions.html#imguiinputtext",
    "kind": "function",
    "text": "Text input widget, returns the current widget buffer content."
  },
  {
    "title": "ImGuiInputVec2",
    "href": "api/lua/functions.html#imguiinputvec2",
    "kind": "function",
    "text": "Vec2 field widget."
  },
  {
    "title": "ImGuiInputVec3",
    "href": "api/lua/functions.html#imguiinputvec3",
    "kind": "function",
    "text": "Vec3 field widget."
  },
  {
    "title": "ImGuiInputVec4",
    "href": "api/lua/functions.html#imguiinputvec4",
    "kind": "function",
    "text": "Vec4 field widget."
  },
  {
    "title": "ImGuiInvisibleButton",
    "href": "api/lua/functions.html#imguiinvisiblebutton",
    "kind": "function",
    "text": "Invisible button widget, return `True` if the button was pressed."
  },
  {
    "title": "ImGuiIsAnyItemActive",
    "href": "api/lua/functions.html#imguiisanyitemactive",
    "kind": "function",
    "text": "Return `true` if any item is active, `false` otherwise."
  },
  {
    "title": "ImGuiIsAnyItemHovered",
    "href": "api/lua/functions.html#imguiisanyitemhovered",
    "kind": "function",
    "text": "Return `true` if any item is hovered by the mouse cursor, `false` otherwise."
  },
  {
    "title": "ImGuiIsItemActive",
    "href": "api/lua/functions.html#imguiisitemactive",
    "kind": "function",
    "text": "Was the last item active. e.g. button being held, text field being edited - items that do not interact will always return `false`."
  },
  {
    "title": "ImGuiIsItemClicked",
    "href": "api/lua/functions.html#imguiisitemclicked",
    "kind": "function",
    "text": "Was the last item clicked."
  },
  {
    "title": "ImGuiIsItemHovered",
    "href": "api/lua/functions.html#imguiisitemhovered",
    "kind": "function",
    "text": "Was the last item hovered by mouse."
  },
  {
    "title": "ImGuiIsItemVisible",
    "href": "api/lua/functions.html#imguiisitemvisible",
    "kind": "function",
    "text": "Was the last item visible and not out of sight due to clipping/scrolling."
  },
  {
    "title": "ImGuiIsKeyDown",
    "href": "api/lua/functions.html#imguiiskeydown",
    "kind": "function",
    "text": "Was the specified key down during the last frame?"
  },
  {
    "title": "ImGuiIsKeyPressed",
    "href": "api/lua/functions.html#imguiiskeypressed",
    "kind": "function",
    "text": "Was the specified key pressed? A key press implies that the key was down and is currently released."
  },
  {
    "title": "ImGuiIsKeyReleased",
    "href": "api/lua/functions.html#imguiiskeyreleased",
    "kind": "function",
    "text": "Was the specified key released during the last frame?"
  },
  {
    "title": "ImGuiIsMouseClicked",
    "href": "api/lua/functions.html#imguiismouseclicked",
    "kind": "function",
    "text": "Was the specified mouse button clicked during the last frame? A mouse click implies that the button pressed earlier and released during the last frame."
  },
  {
    "title": "ImGuiIsMouseDoubleClicked",
    "href": "api/lua/functions.html#imguiismousedoubleclicked",
    "kind": "function",
    "text": "Was the specified mouse button double-clicked during the last frame? A double-click implies two rapid successive clicks of the same button with the mouse cursor staying in the same position."
  },
  {
    "title": "ImGuiIsMouseDown",
    "href": "api/lua/functions.html#imguiismousedown",
    "kind": "function",
    "text": "Was the specified mouse button down during the last frame?"
  },
  {
    "title": "ImGuiIsMouseDragging",
    "href": "api/lua/functions.html#imguiismousedragging",
    "kind": "function",
    "text": "Is mouse dragging?"
  },
  {
    "title": "ImGuiIsMouseHoveringRect",
    "href": "api/lua/functions.html#imguiismousehoveringrect",
    "kind": "function",
    "text": "Test whether the mouse cursor is hovering the specified rectangle."
  },
  {
    "title": "ImGuiIsMouseReleased",
    "href": "api/lua/functions.html#imguiismousereleased",
    "kind": "function",
    "text": "Was the specified mouse button released during the last frame?"
  },
  {
    "title": "ImGuiIsRectVisible",
    "href": "api/lua/functions.html#imguiisrectvisible",
    "kind": "function",
    "text": "Test if a rectangle of the specified size starting from cursor position is visible/not clipped. Or test if a rectangle in screen space is visible/not clipped."
  },
  {
    "title": "ImGuiIsWindowCollapsed",
    "href": "api/lua/functions.html#imguiiswindowcollapsed",
    "kind": "function",
    "text": "Is the current window collapsed."
  },
  {
    "title": "ImGuiIsWindowFocused",
    "href": "api/lua/functions.html#imguiiswindowfocused",
    "kind": "function",
    "text": "Is the current window focused."
  },
  {
    "title": "ImGuiIsWindowHovered",
    "href": "api/lua/functions.html#imguiiswindowhovered",
    "kind": "function",
    "text": "Is the current window hovered and hoverable (not blocked by a popup), differentiates child windows from each others."
  },
  {
    "title": "ImGuiLabelText",
    "href": "api/lua/functions.html#imguilabeltext",
    "kind": "function",
    "text": "Display text+label aligned the same way as value+label widgets."
  },
  {
    "title": "ImGuiListBox",
    "href": "api/lua/functions.html#imguilistbox",
    "kind": "function",
    "text": "List widget."
  },
  {
    "title": "ImGuiMenuItem",
    "href": "api/lua/functions.html#imguimenuitem",
    "kind": "function",
    "text": "Return `true` when activated. Shortcuts are displayed for convenience but not processed at the moment."
  },
  {
    "title": "ImGuiMouseDrawCursor",
    "href": "api/lua/functions.html#imguimousedrawcursor",
    "kind": "function",
    "text": "Enable/disable the ImGui software mouse cursor."
  },
  {
    "title": "ImGuiNewFrame",
    "href": "api/lua/functions.html#imguinewframe",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiNewLine",
    "href": "api/lua/functions.html#imguinewline",
    "kind": "function",
    "text": "Undo a ImGuiSameLine call or force a new line when in an horizontal layout."
  },
  {
    "title": "ImGuiNextColumn",
    "href": "api/lua/functions.html#imguinextcolumn",
    "kind": "function",
    "text": "Start the next column in multi-column layout. See ImGuiColumns."
  },
  {
    "title": "ImGuiOpenPopup",
    "href": "api/lua/functions.html#imguiopenpopup",
    "kind": "function",
    "text": "Mark a named popup as open. Popup windows are closed when the user: * Clicks outside of their client rect, * Activates a pressable item, * ImGuiCloseCurrentPopup is called within a ImGuiBeginPopup/ImGuiEndPopup block. Popup identifiers are "
  },
  {
    "title": "ImGuiPopAllowKeyboardFocus",
    "href": "api/lua/functions.html#imguipopallowkeyboardfocus",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushAllowKeyboardFocus."
  },
  {
    "title": "ImGuiPopButtonRepeat",
    "href": "api/lua/functions.html#imguipopbuttonrepeat",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushButtonRepeat."
  },
  {
    "title": "ImGuiPopClipRect",
    "href": "api/lua/functions.html#imguipopcliprect",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushClipRect."
  },
  {
    "title": "ImGuiPopFont",
    "href": "api/lua/functions.html#imguipopfont",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushFont."
  },
  {
    "title": "ImGuiPopID",
    "href": "api/lua/functions.html#imguipopid",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushID."
  },
  {
    "title": "ImGuiPopItemWidth",
    "href": "api/lua/functions.html#imguipopitemwidth",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushItemWidth."
  },
  {
    "title": "ImGuiPopStyleColor",
    "href": "api/lua/functions.html#imguipopstylecolor",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushStyleColor."
  },
  {
    "title": "ImGuiPopStyleVar",
    "href": "api/lua/functions.html#imguipopstylevar",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushStyleVar."
  },
  {
    "title": "ImGuiPopTextWrapPos",
    "href": "api/lua/functions.html#imguipoptextwrappos",
    "kind": "function",
    "text": "Undo the last call to ImGuiPushTextWrapPos."
  },
  {
    "title": "ImGuiProgressBar",
    "href": "api/lua/functions.html#imguiprogressbar",
    "kind": "function",
    "text": "Draw a progress bar, `fraction` must be between 0.0 and 1.0."
  },
  {
    "title": "ImGuiPushAllowKeyboardFocus",
    "href": "api/lua/functions.html#imguipushallowkeyboardfocus",
    "kind": "function",
    "text": "Allow focusing using TAB/Shift-TAB, enabled by default but you can disable it for certain widgets."
  },
  {
    "title": "ImGuiPushButtonRepeat",
    "href": "api/lua/functions.html#imguipushbuttonrepeat",
    "kind": "function",
    "text": "In repeat mode, `ButtonXXX` functions return repeated true in a typematic manner. Note that you can call ImGuiIsItemActive after any `Button` to tell if the button is held in the current frame."
  },
  {
    "title": "ImGuiPushClipRect",
    "href": "api/lua/functions.html#imguipushcliprect",
    "kind": "function",
    "text": "Push a new clip rectangle onto the clipping stack."
  },
  {
    "title": "ImGuiPushFont",
    "href": "api/lua/functions.html#imguipushfont",
    "kind": "function",
    "text": "Push a font on top of the font stack and make it current for subsequent text rendering operations."
  },
  {
    "title": "ImGuiPushID",
    "href": "api/lua/functions.html#imguipushid",
    "kind": "function",
    "text": "Push a string into the ID stack."
  },
  {
    "title": "ImGuiPushItemWidth",
    "href": "api/lua/functions.html#imguipushitemwidth",
    "kind": "function",
    "text": "Set the width of items for common large `item+label` widgets. - `>0`: width in pixels - `<0`: align `x` pixels to the right of window (so -1 always align width to the right side) - `=0`: default to ~2/3 of the window width See ImGuiPopItemW"
  },
  {
    "title": "ImGuiPushStyleColor",
    "href": "api/lua/functions.html#imguipushstylecolor",
    "kind": "function",
    "text": "Push a value on the style stack for the specified style color. See ImGuiPopStyleColor."
  },
  {
    "title": "ImGuiPushStyleVar",
    "href": "api/lua/functions.html#imguipushstylevar",
    "kind": "function",
    "text": "Push a value on the style stack for the specified style variable. See ImGuiPopStyleVar."
  },
  {
    "title": "ImGuiPushTextWrapPos",
    "href": "api/lua/functions.html#imguipushtextwrappos",
    "kind": "function",
    "text": "Push word-wrapping position for text commands. - ` 0`: Wrap at `wrap_pos_x` position in window local space. See ImGuiPopTextWrapPos."
  },
  {
    "title": "ImGuiRadioButton",
    "href": "api/lua/functions.html#imguiradiobutton",
    "kind": "function",
    "text": "Radio button widget, return the button state."
  },
  {
    "title": "ImGuiRender",
    "href": "api/lua/functions.html#imguirender",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiResetMouseDragDelta",
    "href": "api/lua/functions.html#imguiresetmousedragdelta",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiSameLine",
    "href": "api/lua/functions.html#imguisameline",
    "kind": "function",
    "text": "Call between widgets or groups to layout them horizontally."
  },
  {
    "title": "ImGuiSelectable",
    "href": "api/lua/functions.html#imguiselectable",
    "kind": "function",
    "text": "Selectable item. The following `width` values are possible: * `= 0.0`: Use remaining width. * `> 0.0`: Specific width. The following `height` values are possible: * `= 0.0`: Use label height. * `> 0.0`: Specific height."
  },
  {
    "title": "ImGuiSeparator",
    "href": "api/lua/functions.html#imguiseparator",
    "kind": "function",
    "text": "Output an horizontal line to separate two distinct UI sections."
  },
  {
    "title": "ImGuiSetColumnOffset",
    "href": "api/lua/functions.html#imguisetcolumnoffset",
    "kind": "function",
    "text": "Set the position of a column line in pixels, from the left side of the contents region."
  },
  {
    "title": "ImGuiSetColumnWidth",
    "href": "api/lua/functions.html#imguisetcolumnwidth",
    "kind": "function",
    "text": "Set the column width in pixels."
  },
  {
    "title": "ImGuiSetCursorPos",
    "href": "api/lua/functions.html#imguisetcursorpos",
    "kind": "function",
    "text": "Set the current widget output cursor position in window space."
  },
  {
    "title": "ImGuiSetCursorPosX",
    "href": "api/lua/functions.html#imguisetcursorposx",
    "kind": "function",
    "text": "See ImGuiSetCursorPos."
  },
  {
    "title": "ImGuiSetCursorPosY",
    "href": "api/lua/functions.html#imguisetcursorposy",
    "kind": "function",
    "text": "See ImGuiSetCursorPos."
  },
  {
    "title": "ImGuiSetCursorScreenPos",
    "href": "api/lua/functions.html#imguisetcursorscreenpos",
    "kind": "function",
    "text": "Set the widget cursor output position in screen space."
  },
  {
    "title": "ImGuiSetItemAllowOverlap",
    "href": "api/lua/functions.html#imguisetitemallowoverlap",
    "kind": "function",
    "text": "Allow the last item to be overlapped by a subsequent item. Sometimes useful with invisible buttons, selectables, etc... to catch unused areas."
  },
  {
    "title": "ImGuiSetItemDefaultFocus",
    "href": "api/lua/functions.html#imguisetitemdefaultfocus",
    "kind": "function",
    "text": "Make the last item the default focused item of a window."
  },
  {
    "title": "ImGuiSetKeyboardFocusHere",
    "href": "api/lua/functions.html#imguisetkeyboardfocushere",
    "kind": "function",
    "text": "Focus keyboard on the next widget. Use positive `offset` value to access sub components of a multiple component widget. Use `-1` to access the previous widget."
  },
  {
    "title": "ImGuiSetNextItemOpen",
    "href": "api/lua/functions.html#imguisetnextitemopen",
    "kind": "function",
    "text": "Set next item open state."
  },
  {
    "title": "ImGuiSetNextWindowCollapsed",
    "href": "api/lua/functions.html#imguisetnextwindowcollapsed",
    "kind": "function",
    "text": "Set next window collapsed state, call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowContentSize",
    "href": "api/lua/functions.html#imguisetnextwindowcontentsize",
    "kind": "function",
    "text": "Set the size of the content area of the next declared window. Call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowContentWidth",
    "href": "api/lua/functions.html#imguisetnextwindowcontentwidth",
    "kind": "function",
    "text": "See ImGuiSetNextWindowContentSize."
  },
  {
    "title": "ImGuiSetNextWindowFocus",
    "href": "api/lua/functions.html#imguisetnextwindowfocus",
    "kind": "function",
    "text": "Set the next window to be focused/top-most. Call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowPos",
    "href": "api/lua/functions.html#imguisetnextwindowpos",
    "kind": "function",
    "text": "Set next window position, call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowPosCenter",
    "href": "api/lua/functions.html#imguisetnextwindowposcenter",
    "kind": "function",
    "text": "Set next window position to be centered on screen, call before ImGuiBegin."
  },
  {
    "title": "ImGuiSetNextWindowSize",
    "href": "api/lua/functions.html#imguisetnextwindowsize",
    "kind": "function",
    "text": "Set next window size, call before ImGuiBegin. A value of 0 for an axis will auto-fit it."
  },
  {
    "title": "ImGuiSetNextWindowSizeConstraints",
    "href": "api/lua/functions.html#imguisetnextwindowsizeconstraints",
    "kind": "function",
    "text": "Set the next window size limits. Use -1,-1 on either X/Y axis to preserve the current size. Sizes will be rounded down."
  },
  {
    "title": "ImGuiSetScrollFromPosY",
    "href": "api/lua/functions.html#imguisetscrollfromposy",
    "kind": "function",
    "text": "Adjust scrolling amount to make a given position visible. Generally ImGuiGetCursorStartPos + offset to compute a valid position."
  },
  {
    "title": "ImGuiSetScrollHereY",
    "href": "api/lua/functions.html#imguisetscrollherey",
    "kind": "function",
    "text": "Adjust scrolling amount to make current cursor position visible. - 0: Top. - 0.5: Center. - 1: Bottom. When using to make a default/current item visible, consider using ImGuiSetItemDefaultFocus instead."
  },
  {
    "title": "ImGuiSetScrollX",
    "href": "api/lua/functions.html#imguisetscrollx",
    "kind": "function",
    "text": "Set scrolling amount between [0;ImGuiGetScrollMaxX]."
  },
  {
    "title": "ImGuiSetScrollY",
    "href": "api/lua/functions.html#imguisetscrolly",
    "kind": "function",
    "text": "Set scrolling amount between [0;ImGuiGetScrollMaxY]."
  },
  {
    "title": "ImGuiSetTooltip",
    "href": "api/lua/functions.html#imguisettooltip",
    "kind": "function",
    "text": "Set tooltip under mouse-cursor, typically used with ImGuiIsItemHovered/ImGuiIsAnyItemHovered. Last call wins."
  },
  {
    "title": "ImGuiSetWindowCollapsed",
    "href": "api/lua/functions.html#imguisetwindowcollapsed",
    "kind": "function",
    "text": "Set named window collapsed state, prefer using ImGuiSetNextWindowCollapsed."
  },
  {
    "title": "ImGuiSetWindowFocus",
    "href": "api/lua/functions.html#imguisetwindowfocus",
    "kind": "function",
    "text": "Set named window to be focused/top-most."
  },
  {
    "title": "ImGuiSetWindowFontScale",
    "href": "api/lua/functions.html#imguisetwindowfontscale",
    "kind": "function",
    "text": "Per-window font scale."
  },
  {
    "title": "ImGuiSetWindowPos",
    "href": "api/lua/functions.html#imguisetwindowpos",
    "kind": "function",
    "text": "Set named window position."
  },
  {
    "title": "ImGuiSetWindowSize",
    "href": "api/lua/functions.html#imguisetwindowsize",
    "kind": "function",
    "text": "Set named window size."
  },
  {
    "title": "ImGuiShutdown",
    "href": "api/lua/functions.html#imguishutdown",
    "kind": "function",
    "text": "Shutdown the global ImGui context."
  },
  {
    "title": "ImGuiSliderFloat",
    "href": "api/lua/functions.html#imguisliderfloat",
    "kind": "function",
    "text": "Float slider widget."
  },
  {
    "title": "ImGuiSliderInt",
    "href": "api/lua/functions.html#imguisliderint",
    "kind": "function",
    "text": "Integer slider widget."
  },
  {
    "title": "ImGuiSliderIntVec2",
    "href": "api/lua/functions.html#imguisliderintvec2",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ImGuiSliderVec2",
    "href": "api/lua/functions.html#imguislidervec2",
    "kind": "function",
    "text": "Vec2 slider widget."
  },
  {
    "title": "ImGuiSliderVec3",
    "href": "api/lua/functions.html#imguislidervec3",
    "kind": "function",
    "text": "Vec3 slider widget."
  },
  {
    "title": "ImGuiSliderVec4",
    "href": "api/lua/functions.html#imguislidervec4",
    "kind": "function",
    "text": "Vec4 slider widget."
  },
  {
    "title": "ImGuiSmallButton",
    "href": "api/lua/functions.html#imguismallbutton",
    "kind": "function",
    "text": "Small button widget fitting the height of a text line, return `True` if the button was pressed."
  },
  {
    "title": "ImGuiSpacing",
    "href": "api/lua/functions.html#imguispacing",
    "kind": "function",
    "text": "Add spacing."
  },
  {
    "title": "ImGuiText",
    "href": "api/lua/functions.html#imguitext",
    "kind": "function",
    "text": "Static text."
  },
  {
    "title": "ImGuiTextColored",
    "href": "api/lua/functions.html#imguitextcolored",
    "kind": "function",
    "text": "Colored static text."
  },
  {
    "title": "ImGuiTextDisabled",
    "href": "api/lua/functions.html#imguitextdisabled",
    "kind": "function",
    "text": "Disabled static text."
  },
  {
    "title": "ImGuiTextUnformatted",
    "href": "api/lua/functions.html#imguitextunformatted",
    "kind": "function",
    "text": "Raw text without formatting. Roughly equivalent to ImGuiText but faster, recommended for long chunks of text."
  },
  {
    "title": "ImGuiTextWrapped",
    "href": "api/lua/functions.html#imguitextwrapped",
    "kind": "function",
    "text": "Wrapped static text. Note that this won't work on an auto-resizing window if there's no other widgets to extend the window width, you may need to set a size using ImGuiSetNextWindowSize."
  },
  {
    "title": "ImGuiTreeNode",
    "href": "api/lua/functions.html#imguitreenode",
    "kind": "function",
    "text": "If returning `true` the node is open and the user is responsible for calling ImGuiTreePop."
  },
  {
    "title": "ImGuiTreeNodeEx",
    "href": "api/lua/functions.html#imguitreenodeex",
    "kind": "function",
    "text": "See ImGuiTreeNode."
  },
  {
    "title": "ImGuiTreePop",
    "href": "api/lua/functions.html#imguitreepop",
    "kind": "function",
    "text": "Pop the current tree node."
  },
  {
    "title": "ImGuiTreePush",
    "href": "api/lua/functions.html#imguitreepush",
    "kind": "function",
    "text": "Already called by ImGuiTreeNode, but you can call ImGuiTreePush/ImGuiTreePop yourself for layouting purpose."
  },
  {
    "title": "ImGuiUnindent",
    "href": "api/lua/functions.html#imguiunindent",
    "kind": "function",
    "text": "Move content position back to the left (cancel ImGuiIndent)."
  },
  {
    "title": "ImGuiWantCaptureMouse",
    "href": "api/lua/functions.html#imguiwantcapturemouse",
    "kind": "function",
    "text": "ImGui wants mouse capture. Use this function to determine when to pause mouse processing from other parts of your program."
  },
  {
    "title": "Inch",
    "href": "api/lua/functions.html#inch",
    "kind": "function",
    "text": "Convert a value in inches to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "InputInit",
    "href": "api/lua/functions.html#inputinit",
    "kind": "function",
    "text": "Initialize the Input system. Must be invoked before any call to WindowSystemInit to work properly. ```python hg.InputInit() hg.WindowSystemInit() ```"
  },
  {
    "title": "InputShutdown",
    "href": "api/lua/functions.html#inputshutdown",
    "kind": "function",
    "text": "Shutdown the Input system."
  },
  {
    "title": "Inside",
    "href": "api/lua/functions.html#inside",
    "kind": "function",
    "text": "Test if a value is inside a containing volume."
  },
  {
    "title": "int_to_VoidPointer",
    "href": "api/lua/functions.html#int_to_voidpointer",
    "kind": "function",
    "text": "Cast an integer to a void pointer. This function is only used to provide access to low-level structures and should not be needed most of the time."
  },
  {
    "title": "Intersection",
    "href": "api/lua/functions.html#intersection",
    "kind": "function",
    "text": "Return the intersection of two rectangles."
  },
  {
    "title": "IntersectRay",
    "href": "api/lua/functions.html#intersectray",
    "kind": "function",
    "text": "Intersect an infinite ray with an axis-aligned bounding box, if the first returned value is `true` it is followed by the near and far intersection points."
  },
  {
    "title": "Intersects",
    "href": "api/lua/functions.html#intersects",
    "kind": "function",
    "text": "Return `true` if rect `a` intersects rect `b`."
  },
  {
    "title": "Inverse",
    "href": "api/lua/functions.html#inverse",
    "kind": "function",
    "text": "Return the inverse of a matrix, vector or quaternion."
  },
  {
    "title": "InverseFast",
    "href": "api/lua/functions.html#inversefast",
    "kind": "function",
    "text": "Compute the inverse of an orthonormal transformation matrix. This function is faster than the generic Inverse function but can only deal with a specific set of matrices. See Inverse."
  },
  {
    "title": "IsAssetFile",
    "href": "api/lua/functions.html#isassetfile",
    "kind": "function",
    "text": "Test if an asset file exists in the assets system. See man.Assets."
  },
  {
    "title": "IsDir",
    "href": "api/lua/functions.html#isdir",
    "kind": "function",
    "text": "Returns `true` if `path` is a directory on the local filesystem, `false` otherwise."
  },
  {
    "title": "IsEOF",
    "href": "api/lua/functions.html#iseof",
    "kind": "function",
    "text": "Returns `true` if the cursor is at the end of the file, `false` otherwise."
  },
  {
    "title": "IsFile",
    "href": "api/lua/functions.html#isfile",
    "kind": "function",
    "text": "Test if a file exists on the local filesystem."
  },
  {
    "title": "IsFinite",
    "href": "api/lua/functions.html#isfinite",
    "kind": "function",
    "text": "Test if a floating point value is finite."
  },
  {
    "title": "IsHandJointActive",
    "href": "api/lua/functions.html#ishandjointactive",
    "kind": "function",
    "text": ""
  },
  {
    "title": "IsMonitorConnected",
    "href": "api/lua/functions.html#ismonitorconnected",
    "kind": "function",
    "text": "Test if the specified monitor is connected to the host device."
  },
  {
    "title": "IsoSurfaceSphere",
    "href": "api/lua/functions.html#isosurfacesphere",
    "kind": "function",
    "text": "Output a sphere to an iso-surface."
  },
  {
    "title": "IsoSurfaceToModel",
    "href": "api/lua/functions.html#isosurfacetomodel",
    "kind": "function",
    "text": "Convert an iso-surface to a render model, this function is geared toward efficiency and meant for realtime."
  },
  {
    "title": "IsPathAbsolute",
    "href": "api/lua/functions.html#ispathabsolute",
    "kind": "function",
    "text": "Test if the provided path is an absolute or relative path."
  },
  {
    "title": "IsPrimaryMonitor",
    "href": "api/lua/functions.html#isprimarymonitor",
    "kind": "function",
    "text": "Return `true` if the monitor is the primary host device monitor, `false` otherwise."
  },
  {
    "title": "IsValid",
    "href": "api/lua/functions.html#isvalid",
    "kind": "function",
    "text": "Test if a resource if valid."
  },
  {
    "title": "IsWindowOpen",
    "href": "api/lua/functions.html#iswindowopen",
    "kind": "function",
    "text": "Return `true` if the window is open, `false` otherwise."
  },
  {
    "title": "Km",
    "href": "api/lua/functions.html#km",
    "kind": "function",
    "text": "Convert a value in kilometers to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Len",
    "href": "api/lua/functions.html#len",
    "kind": "function",
    "text": "Return the length of the vector."
  },
  {
    "title": "Len2",
    "href": "api/lua/functions.html#len2",
    "kind": "function",
    "text": "Return the length of the vector squared."
  },
  {
    "title": "Lerp",
    "href": "api/lua/functions.html#lerp",
    "kind": "function",
    "text": "See LinearInterpolate."
  },
  {
    "title": "LerpAsOrthonormalBase",
    "href": "api/lua/functions.html#lerpasorthonormalbase",
    "kind": "function",
    "text": "Linear interpolate between two transformation matrices on the [0;1] interval."
  },
  {
    "title": "LinearInterpolate",
    "href": "api/lua/functions.html#linearinterpolate",
    "kind": "function",
    "text": "Linear interpolate between two values on the [0;1] interval. See CosineInterpolate, CubicInterpolate and HermiteInterpolate."
  },
  {
    "title": "ListDir",
    "href": "api/lua/functions.html#listdir",
    "kind": "function",
    "text": "Get the content of a directory on the local filesystem, this function does not recurse into subfolders. See ListDirRecursive."
  },
  {
    "title": "ListDirRecursive",
    "href": "api/lua/functions.html#listdirrecursive",
    "kind": "function",
    "text": "Get the content of a directory on the local filesystem, this function recurses into subfolders. See ListDir."
  },
  {
    "title": "LoadBMP",
    "href": "api/lua/functions.html#loadbmp",
    "kind": "function",
    "text": "Load a Picture in [BMP](https://en.wikipedia.org/wiki/BMP_file_format) file format."
  },
  {
    "title": "LoadDataFromFile",
    "href": "api/lua/functions.html#loaddatafromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadFontFromAssets",
    "href": "api/lua/functions.html#loadfontfromassets",
    "kind": "function",
    "text": "Load a TrueType (TTF) font from the assets system. See man.Assets."
  },
  {
    "title": "LoadFontFromFile",
    "href": "api/lua/functions.html#loadfontfromfile",
    "kind": "function",
    "text": "Load a TrueType (TTF) font from the local filesystem. See man.Assets."
  },
  {
    "title": "LoadForwardPipelineAAAConfigFromAssets",
    "href": "api/lua/functions.html#loadforwardpipelineaaaconfigfromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadForwardPipelineAAAConfigFromFile",
    "href": "api/lua/functions.html#loadforwardpipelineaaaconfigfromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadGIF",
    "href": "api/lua/functions.html#loadgif",
    "kind": "function",
    "text": "Load a Picture in [GIF](https://en.wikipedia.org/wiki/GIF) file format."
  },
  {
    "title": "LoadJPG",
    "href": "api/lua/functions.html#loadjpg",
    "kind": "function",
    "text": "Load a Picture in [JPEG](https://en.wikipedia.org/wiki/JPEG) file format."
  },
  {
    "title": "LoadJsonFromAssets",
    "href": "api/lua/functions.html#loadjsonfromassets",
    "kind": "function",
    "text": "Load a JSON from the assets system. See man.Assets."
  },
  {
    "title": "LoadJsonFromFile",
    "href": "api/lua/functions.html#loadjsonfromfile",
    "kind": "function",
    "text": "Load a JSON from the local filesystem."
  },
  {
    "title": "LoadModelFromAssets",
    "href": "api/lua/functions.html#loadmodelfromassets",
    "kind": "function",
    "text": "Load a render model from the assets system. See DrawModel and man.Assets."
  },
  {
    "title": "LoadModelFromFile",
    "href": "api/lua/functions.html#loadmodelfromfile",
    "kind": "function",
    "text": "Load a render model from the local filesystem."
  },
  {
    "title": "LoadNavMeshFromAssets",
    "href": "api/lua/functions.html#loadnavmeshfromassets",
    "kind": "function",
    "text": "Load a navigation mesh from the assets system. See man.Assets."
  },
  {
    "title": "LoadNavMeshFromFile",
    "href": "api/lua/functions.html#loadnavmeshfromfile",
    "kind": "function",
    "text": "Load a navigation mesh from the local filesystem."
  },
  {
    "title": "LoadOGGSoundAsset",
    "href": "api/lua/functions.html#loadoggsoundasset",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadOGGSoundFile",
    "href": "api/lua/functions.html#loadoggsoundfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadPicture",
    "href": "api/lua/functions.html#loadpicture",
    "kind": "function",
    "text": "Load a Picture content from the filesystem."
  },
  {
    "title": "LoadPipelineProgramFromAssets",
    "href": "api/lua/functions.html#loadpipelineprogramfromassets",
    "kind": "function",
    "text": "Load a pipeline shader program from the assets system. See man.Assets."
  },
  {
    "title": "LoadPipelineProgramFromFile",
    "href": "api/lua/functions.html#loadpipelineprogramfromfile",
    "kind": "function",
    "text": "Load a pipeline shader program from the local filesystem."
  },
  {
    "title": "LoadPipelineProgramRefFromAssets",
    "href": "api/lua/functions.html#loadpipelineprogramreffromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadPipelineProgramRefFromFile",
    "href": "api/lua/functions.html#loadpipelineprogramreffromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadPNG",
    "href": "api/lua/functions.html#loadpng",
    "kind": "function",
    "text": "Load a Picture in [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) file format."
  },
  {
    "title": "LoadProgramFromAssets",
    "href": "api/lua/functions.html#loadprogramfromassets",
    "kind": "function",
    "text": "Load a shader program from the assets system. See man.Assets."
  },
  {
    "title": "LoadProgramFromFile",
    "href": "api/lua/functions.html#loadprogramfromfile",
    "kind": "function",
    "text": "Load a shader program from the local filesystem."
  },
  {
    "title": "LoadPSD",
    "href": "api/lua/functions.html#loadpsd",
    "kind": "function",
    "text": "Load a Picture in [PSD](https://en.wikipedia.org/wiki/Adobe_Photoshop#File_format) file format."
  },
  {
    "title": "LoadSceneBinaryFromAssets",
    "href": "api/lua/functions.html#loadscenebinaryfromassets",
    "kind": "function",
    "text": "Load a scene in binary format from the assets system. Loaded content is added to the existing scene content. See man.Assets."
  },
  {
    "title": "LoadSceneBinaryFromDataAndAssets",
    "href": "api/lua/functions.html#loadscenebinaryfromdataandassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadSceneBinaryFromDataAndFile",
    "href": "api/lua/functions.html#loadscenebinaryfromdataandfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadSceneBinaryFromFile",
    "href": "api/lua/functions.html#loadscenebinaryfromfile",
    "kind": "function",
    "text": "Load a scene in binary format from the local filesystem. Loaded content is added to the existing scene content."
  },
  {
    "title": "LoadSceneFromAssets",
    "href": "api/lua/functions.html#loadscenefromassets",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadSceneFromFile",
    "href": "api/lua/functions.html#loadscenefromfile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "LoadSceneJsonFromAssets",
    "href": "api/lua/functions.html#loadscenejsonfromassets",
    "kind": "function",
    "text": "Load a scene in JSON format from the assets system. Loaded content is added to the existing scene content. See man.Assets."
  },
  {
    "title": "LoadSceneJsonFromFile",
    "href": "api/lua/functions.html#loadscenejsonfromfile",
    "kind": "function",
    "text": "Load a scene in JSON format from the local filesystem. Loaded content is added to the existing scene content."
  },
  {
    "title": "LoadTextureFlagsFromAssets",
    "href": "api/lua/functions.html#loadtextureflagsfromassets",
    "kind": "function",
    "text": "Load texture flags in the texture metafile from the assets system. See man.Assets."
  },
  {
    "title": "LoadTextureFlagsFromFile",
    "href": "api/lua/functions.html#loadtextureflagsfromfile",
    "kind": "function",
    "text": "Load texture flags in the texture metafile from the local filesystem."
  },
  {
    "title": "LoadTextureFromAssets",
    "href": "api/lua/functions.html#loadtexturefromassets",
    "kind": "function",
    "text": "Load a texture from the assets system. - When not using pipeline resources the texture informations are returned directly. - When using pipeline resources the texture informations can be retrieved from the PipelineResources object. See man."
  },
  {
    "title": "LoadTextureFromFile",
    "href": "api/lua/functions.html#loadtexturefromfile",
    "kind": "function",
    "text": "Load a texture from the local filesystem. - When not using pipeline resources the texture informations are returned directly. - When using pipeline resources the texture informations can be retrieved from the PipelineResources object."
  },
  {
    "title": "LoadTGA",
    "href": "api/lua/functions.html#loadtga",
    "kind": "function",
    "text": "Load a Picture in [TGA](https://en.wikipedia.org/wiki/Truevision_TGA) file format."
  },
  {
    "title": "LoadWAVSoundAsset",
    "href": "api/lua/functions.html#loadwavsoundasset",
    "kind": "function",
    "text": "Load a sound in WAV format from the assets system and return a reference to it. See man.Assets."
  },
  {
    "title": "LoadWAVSoundFile",
    "href": "api/lua/functions.html#loadwavsoundfile",
    "kind": "function",
    "text": "Load a sound in WAV format from the local filesystem and return a reference to it."
  },
  {
    "title": "Log",
    "href": "api/lua/functions.html#log",
    "kind": "function",
    "text": ""
  },
  {
    "title": "MakeForwardPipelineLinearLight",
    "href": "api/lua/functions.html#makeforwardpipelinelinearlight",
    "kind": "function",
    "text": "Create a forward pipeline linear light. See ForwardPipelineLights, PrepareForwardPipelineLights and SubmitModelToForwardPipeline."
  },
  {
    "title": "MakeForwardPipelinePointLight",
    "href": "api/lua/functions.html#makeforwardpipelinepointlight",
    "kind": "function",
    "text": "Create a forward pipeline point light. See ForwardPipelineLights, PrepareForwardPipelineLights and SubmitModelToForwardPipeline."
  },
  {
    "title": "MakeForwardPipelineSpotLight",
    "href": "api/lua/functions.html#makeforwardpipelinespotlight",
    "kind": "function",
    "text": "Create a forward pipeline spot light. See ForwardPipelineLights, PrepareForwardPipelineLights and SubmitModelToForwardPipeline."
  },
  {
    "title": "MakeFrustum",
    "href": "api/lua/functions.html#makefrustum",
    "kind": "function",
    "text": "Create a projection frustum. This object can then be used to perform culling using TestVisibility. ```python # Compute a perspective matrix proj = hg.ComputePerspectiveProjectionMatrix(0.1, 1000, hg.FovToZoomFactor(math.pi/4), 1280/720) # M"
  },
  {
    "title": "MakePlane",
    "href": "api/lua/functions.html#makeplane",
    "kind": "function",
    "text": "Geometrical plane in 3D space. - `p`: a point lying on the plane. - `n`: the plane normal. - `m`: an affine transformation matrix that will be applied to `p` and `n`."
  },
  {
    "title": "MakeRectFromWidthHeight",
    "href": "api/lua/functions.html#makerectfromwidthheight",
    "kind": "function",
    "text": "Make a rectangle from width and height."
  },
  {
    "title": "MakeUniformSetTexture",
    "href": "api/lua/functions.html#makeuniformsettexture",
    "kind": "function",
    "text": "Create a uniform set texture object. This object can be added to a UniformSetTextureList to control the shader program uniform values for a subsequent call to DrawModel."
  },
  {
    "title": "MakeUniformSetValue",
    "href": "api/lua/functions.html#makeuniformsetvalue",
    "kind": "function",
    "text": "Create a uniform set value object. This object can be added to a UniformSetValueList to control the shader program uniform values for a subsequent call to DrawModel."
  },
  {
    "title": "MakeVec3",
    "href": "api/lua/functions.html#makevec3",
    "kind": "function",
    "text": "Make a Vec3 from a Vec4. The input vector `w` component is discarded."
  },
  {
    "title": "MakeVertex",
    "href": "api/lua/functions.html#makevertex",
    "kind": "function",
    "text": ""
  },
  {
    "title": "MakeVideoStreamer",
    "href": "api/lua/functions.html#makevideostreamer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Mat3LookAt",
    "href": "api/lua/functions.html#mat3lookat",
    "kind": "function",
    "text": "Return a rotation matrix looking down the provided vector. The input vector does not need to be normalized."
  },
  {
    "title": "Mat4LookAt",
    "href": "api/lua/functions.html#mat4lookat",
    "kind": "function",
    "text": "Return a _look at_ matrix whose orientation points at the specified position."
  },
  {
    "title": "Mat4LookAtUp",
    "href": "api/lua/functions.html#mat4lookatup",
    "kind": "function",
    "text": "Return a _look at_ matrix whose orientation points at the specified position and up direction."
  },
  {
    "title": "Mat4LookToward",
    "href": "api/lua/functions.html#mat4looktoward",
    "kind": "function",
    "text": "Return a _look at_ matrix whose orientation points toward the specified direction."
  },
  {
    "title": "Mat4LookTowardUp",
    "href": "api/lua/functions.html#mat4looktowardup",
    "kind": "function",
    "text": "Return a _look at_ matrix whose orientation points toward the specified directions."
  },
  {
    "title": "Max",
    "href": "api/lua/functions.html#max",
    "kind": "function",
    "text": "Return a vector whose elements are the maximum of each of the two specified vectors."
  },
  {
    "title": "Min",
    "href": "api/lua/functions.html#min",
    "kind": "function",
    "text": "Return a vector whose elements are the minimum of each of the two specified vectors."
  },
  {
    "title": "MinMaxFromPositionSize",
    "href": "api/lua/functions.html#minmaxfrompositionsize",
    "kind": "function",
    "text": "Set `min = p - size/2` and `max = p + size/2`."
  },
  {
    "title": "MkDir",
    "href": "api/lua/functions.html#mkdir",
    "kind": "function",
    "text": "Create a new directory. See MkTree."
  },
  {
    "title": "MkTree",
    "href": "api/lua/functions.html#mktree",
    "kind": "function",
    "text": "Create a directory tree on the local filesystem. This function is recursive and creates each missing directory in the path. See MkDir."
  },
  {
    "title": "Mm",
    "href": "api/lua/functions.html#mm",
    "kind": "function",
    "text": "Convert a value in millimeters to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Ms",
    "href": "api/lua/functions.html#ms",
    "kind": "function",
    "text": "Convert a value in milliseconds to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Mtr",
    "href": "api/lua/functions.html#mtr",
    "kind": "function",
    "text": "Convert a value in meters to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "NewFullscreenWindow",
    "href": "api/lua/functions.html#newfullscreenwindow",
    "kind": "function",
    "text": "Create a new fullscreen window."
  },
  {
    "title": "NewIsoSurface",
    "href": "api/lua/functions.html#newisosurface",
    "kind": "function",
    "text": "Return a new iso-surface object. See IsoSurfaceSphere to draw to an iso-surface and IsoSurfaceToModel to draw it."
  },
  {
    "title": "NewWindow",
    "href": "api/lua/functions.html#newwindow",
    "kind": "function",
    "text": "Create a new window."
  },
  {
    "title": "NewWindowFrom",
    "href": "api/lua/functions.html#newwindowfrom",
    "kind": "function",
    "text": "Wrap a native window handle in a Window object."
  },
  {
    "title": "Normalize",
    "href": "api/lua/functions.html#normalize",
    "kind": "function",
    "text": "Return the input vector scaled so that its length is one."
  },
  {
    "title": "NormalizePath",
    "href": "api/lua/functions.html#normalizepath",
    "kind": "function",
    "text": "Normalize a path according to the following conventions: - Replace all whitespaces by underscores."
  },
  {
    "title": "Offset",
    "href": "api/lua/functions.html#offset",
    "kind": "function",
    "text": "Offset a rectangle by the specified amount of units."
  },
  {
    "title": "Open",
    "href": "api/lua/functions.html#open",
    "kind": "function",
    "text": "Open a file in binary mode. See OpenText, OpenWrite, OpenWriteText"
  },
  {
    "title": "OpenFileDialog",
    "href": "api/lua/functions.html#openfiledialog",
    "kind": "function",
    "text": "Open a native OpenFile dialog."
  },
  {
    "title": "OpenFolderDialog",
    "href": "api/lua/functions.html#openfolderdialog",
    "kind": "function",
    "text": "Open a native OpenFolder dialog."
  },
  {
    "title": "OpenTemp",
    "href": "api/lua/functions.html#opentemp",
    "kind": "function",
    "text": "Return a handle to a temporary file on the local filesystem."
  },
  {
    "title": "OpenText",
    "href": "api/lua/functions.html#opentext",
    "kind": "function",
    "text": "Open a file as text. Return a handle to the opened file. See Open, OpenWrite, OpenWriteText"
  },
  {
    "title": "OpenVRCreateEyeFrameBuffer",
    "href": "api/lua/functions.html#openvrcreateeyeframebuffer",
    "kind": "function",
    "text": "Creates and returns an man.VR eye framebuffer, with the desired level of anti-aliasing. This function must be invoked twice, for the left and right eyes."
  },
  {
    "title": "OpenVRDestroyEyeFrameBuffer",
    "href": "api/lua/functions.html#openvrdestroyeyeframebuffer",
    "kind": "function",
    "text": "Destroy an eye framebuffer."
  },
  {
    "title": "OpenVRGetColorTexture",
    "href": "api/lua/functions.html#openvrgetcolortexture",
    "kind": "function",
    "text": "Return the color texture attached to an eye framebuffer."
  },
  {
    "title": "OpenVRGetDepthTexture",
    "href": "api/lua/functions.html#openvrgetdepthtexture",
    "kind": "function",
    "text": "Return the depth texture attached to an eye framebuffer."
  },
  {
    "title": "OpenVRGetFrameBufferSize",
    "href": "api/lua/functions.html#openvrgetframebuffersize",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenVRGetState",
    "href": "api/lua/functions.html#openvrgetstate",
    "kind": "function",
    "text": "Returns the current OpenVR state including the body, head and eye transformations."
  },
  {
    "title": "OpenVRInit",
    "href": "api/lua/functions.html#openvrinit",
    "kind": "function",
    "text": "Initialize OpenVR. Start the device display, its controllers and trackers."
  },
  {
    "title": "OpenVRIsHMDMounted",
    "href": "api/lua/functions.html#openvrishmdmounted",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenVRPostPresentHandoff",
    "href": "api/lua/functions.html#openvrpostpresenthandoff",
    "kind": "function",
    "text": "Signal to the OpenVR compositor that it can immediatly start processing the current frame."
  },
  {
    "title": "OpenVRShutdown",
    "href": "api/lua/functions.html#openvrshutdown",
    "kind": "function",
    "text": "Shutdown OpenVR."
  },
  {
    "title": "OpenVRStateToViewState",
    "href": "api/lua/functions.html#openvrstatetoviewstate",
    "kind": "function",
    "text": "Compute the left and right eye view states from an OpenVR state. See OpenVRGetState."
  },
  {
    "title": "OpenVRSubmitFrame",
    "href": "api/lua/functions.html#openvrsubmitframe",
    "kind": "function",
    "text": "Submit the left and right eye textures to the OpenVR compositor. See OpenVRCreateEyeFrameBuffer."
  },
  {
    "title": "OpenWrite",
    "href": "api/lua/functions.html#openwrite",
    "kind": "function",
    "text": "Open a file as binary in write mode. See Open, OpenText, OpenWriteText"
  },
  {
    "title": "OpenWriteText",
    "href": "api/lua/functions.html#openwritetext",
    "kind": "function",
    "text": "Open a file as text in write mode. See Open, OpenText, OpenWrite"
  },
  {
    "title": "OpenXRCreateEyeFrameBuffer",
    "href": "api/lua/functions.html#openxrcreateeyeframebuffer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRDestroyEyeFrameBuffer",
    "href": "api/lua/functions.html#openxrdestroyeyeframebuffer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRFinishSubmitFrameBuffer",
    "href": "api/lua/functions.html#openxrfinishsubmitframebuffer",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetColorTexture",
    "href": "api/lua/functions.html#openxrgetcolortexture",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetColorTextureFromId",
    "href": "api/lua/functions.html#openxrgetcolortexturefromid",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetDepthTexture",
    "href": "api/lua/functions.html#openxrgetdepthtexture",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetDepthTextureFromId",
    "href": "api/lua/functions.html#openxrgetdepthtexturefromid",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetEyeGaze",
    "href": "api/lua/functions.html#openxrgeteyegaze",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetHeadPose",
    "href": "api/lua/functions.html#openxrgetheadpose",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRGetInstanceInfo",
    "href": "api/lua/functions.html#openxrgetinstanceinfo",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRInit",
    "href": "api/lua/functions.html#openxrinit",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRShutdown",
    "href": "api/lua/functions.html#openxrshutdown",
    "kind": "function",
    "text": ""
  },
  {
    "title": "OpenXRSubmitSceneToForwardPipeline",
    "href": "api/lua/functions.html#openxrsubmitscenetoforwardpipeline",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Orthonormalize",
    "href": "api/lua/functions.html#orthonormalize",
    "kind": "function",
    "text": "Return a matrix where the row vectors form an orthonormal basis. All vectors are normalized and perpendicular to each other."
  },
  {
    "title": "Overlap",
    "href": "api/lua/functions.html#overlap",
    "kind": "function",
    "text": "Return `true` if the provided volume overlaps with this volume, `false` otherwise. The test can optionally be restricted to a specific axis."
  },
  {
    "title": "PathJoin",
    "href": "api/lua/functions.html#pathjoin",
    "kind": "function",
    "text": "Return a file path from a set of string elements."
  },
  {
    "title": "PathStartsWith",
    "href": "api/lua/functions.html#pathstartswith",
    "kind": "function",
    "text": "Test if the provided path starts with the provided prefix."
  },
  {
    "title": "PathStripPrefix",
    "href": "api/lua/functions.html#pathstripprefix",
    "kind": "function",
    "text": "Return a copy of the input path stripped of the provided prefix."
  },
  {
    "title": "PathStripSuffix",
    "href": "api/lua/functions.html#pathstripsuffix",
    "kind": "function",
    "text": "Return a copy of the input path stripped of the provided suffix."
  },
  {
    "title": "PathToDisplay",
    "href": "api/lua/functions.html#pathtodisplay",
    "kind": "function",
    "text": "Format a path for display."
  },
  {
    "title": "PauseSource",
    "href": "api/lua/functions.html#pausesource",
    "kind": "function",
    "text": "Pause a playing audio source. See PlayStereo and PlaySpatialized."
  },
  {
    "title": "PlaySpatialized",
    "href": "api/lua/functions.html#playspatialized",
    "kind": "function",
    "text": "Start playing a spatialized sound. Return a handle to the started source."
  },
  {
    "title": "PlayStereo",
    "href": "api/lua/functions.html#playstereo",
    "kind": "function",
    "text": "Start playing a stereo sound. Return a handle to the started source."
  },
  {
    "title": "PrepareForwardPipelineLights",
    "href": "api/lua/functions.html#prepareforwardpipelinelights",
    "kind": "function",
    "text": "Prepare a list of forward pipeline lights into a structure ready for submitting to the forward pipeline. Lights are sorted by priority/type and the most important lights are assigned to available lighting slot of the forward pipeline. See S"
  },
  {
    "title": "PrepareSceneForwardPipelineCommonRenderData",
    "href": "api/lua/functions.html#preparesceneforwardpipelinecommonrenderdata",
    "kind": "function",
    "text": "Prepare the common render data to submit a scene to the forward pipeline. Note: When rendering multiple views of the same scene, common data only needs to be prepared once. See PrepareSceneForwardPipelineViewDependentRenderData."
  },
  {
    "title": "PrepareSceneForwardPipelineViewDependentRenderData",
    "href": "api/lua/functions.html#preparesceneforwardpipelineviewdependentrenderdata",
    "kind": "function",
    "text": "Prepare the view dependent render data to submit a scene to the forward pipeline. See PrepareSceneForwardPipelineCommonRenderData."
  },
  {
    "title": "PrintProfilerFrame",
    "href": "api/lua/functions.html#printprofilerframe",
    "kind": "function",
    "text": "Print a profiler frame to the console. Print all sections in the frame, their duration and event count."
  },
  {
    "title": "ProcessLoadQueues",
    "href": "api/lua/functions.html#processloadqueues",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ProcessModelLoadQueue",
    "href": "api/lua/functions.html#processmodelloadqueue",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ProcessTextureLoadQueue",
    "href": "api/lua/functions.html#processtextureloadqueue",
    "kind": "function",
    "text": "Process the texture load queue. This function must be called to load textures queued while loading a scene or model with the LSSF_QueueTextureLoads flag. See LoadSaveSceneFlags."
  },
  {
    "title": "ProjectOrthoToClipSpace",
    "href": "api/lua/functions.html#projectorthotoclipspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ProjectOrthoToScreenSpace",
    "href": "api/lua/functions.html#projectorthotoscreenspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ProjectToClipSpace",
    "href": "api/lua/functions.html#projecttoclipspace",
    "kind": "function",
    "text": "Project a world position to the clipping space."
  },
  {
    "title": "ProjectToScreenSpace",
    "href": "api/lua/functions.html#projecttoscreenspace",
    "kind": "function",
    "text": "Project a world position to screen coordinates."
  },
  {
    "title": "ProjectZToClipSpace",
    "href": "api/lua/functions.html#projectztoclipspace",
    "kind": "function",
    "text": "Project a depth value to clip space."
  },
  {
    "title": "Quantize",
    "href": "api/lua/functions.html#quantize",
    "kind": "function",
    "text": "Return the provided value quantized to the specified step."
  },
  {
    "title": "QuaternionFromAxisAngle",
    "href": "api/lua/functions.html#quaternionfromaxisangle",
    "kind": "function",
    "text": "Return a quaternion rotation from a 3d axis and a rotation around that axis."
  },
  {
    "title": "QuaternionFromEuler",
    "href": "api/lua/functions.html#quaternionfromeuler",
    "kind": "function",
    "text": "Return a quaternion 3d rotation from its _Euler_ vector representation."
  },
  {
    "title": "QuaternionFromMatrix3",
    "href": "api/lua/functions.html#quaternionfrommatrix3",
    "kind": "function",
    "text": "Return a quaternion rotation from its Mat3 representation."
  },
  {
    "title": "QuaternionLookAt",
    "href": "api/lua/functions.html#quaternionlookat",
    "kind": "function",
    "text": "Return a quaternion 3d rotation oriented toward the specified position when sitting on the world's origin _{0, 0, 0}_."
  },
  {
    "title": "Rad",
    "href": "api/lua/functions.html#rad",
    "kind": "function",
    "text": "Convert an angle in radians to the engine unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Rad3",
    "href": "api/lua/functions.html#rad3",
    "kind": "function",
    "text": "Convert a triplet of angles in radians to the engine unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "RadianToDegree",
    "href": "api/lua/functions.html#radiantodegree",
    "kind": "function",
    "text": "Convert an angle in radians to degrees."
  },
  {
    "title": "Rand",
    "href": "api/lua/functions.html#rand",
    "kind": "function",
    "text": "Return a random integer value in the provided range, default range is [0;65535]. See FRand to generate a random floating point value."
  },
  {
    "title": "RandomVec3",
    "href": "api/lua/functions.html#randomvec3",
    "kind": "function",
    "text": "Return a vector with each component randomized in the inclusive provided range."
  },
  {
    "title": "RandomVec4",
    "href": "api/lua/functions.html#randomvec4",
    "kind": "function",
    "text": "Return a vector with each component randomized in the inclusive provided range."
  },
  {
    "title": "ReadFloat",
    "href": "api/lua/functions.html#readfloat",
    "kind": "function",
    "text": "Read a binary 32 bit floating point value from a local file."
  },
  {
    "title": "ReadGamepad",
    "href": "api/lua/functions.html#readgamepad",
    "kind": "function",
    "text": "Read the current state of a named gamepad. If no name is passed, `default` is implied. See GetGamepadNames."
  },
  {
    "title": "ReadJoystick",
    "href": "api/lua/functions.html#readjoystick",
    "kind": "function",
    "text": ""
  },
  {
    "title": "ReadKeyboard",
    "href": "api/lua/functions.html#readkeyboard",
    "kind": "function",
    "text": "Read the current state of a named keyboard. If no name is passed, `default` is implied. See GetKeyboardNames."
  },
  {
    "title": "ReadMouse",
    "href": "api/lua/functions.html#readmouse",
    "kind": "function",
    "text": "Read the current state of a named mouse. If no name is passed, `default` is implied. See GetMouseNames."
  },
  {
    "title": "ReadString",
    "href": "api/lua/functions.html#readstring",
    "kind": "function",
    "text": "Read a binary string from a local file. Strings are stored as a `uint32_t length` field followed by the string content in UTF-8."
  },
  {
    "title": "ReadUInt16",
    "href": "api/lua/functions.html#readuint16",
    "kind": "function",
    "text": "Read a binary 16 bit unsigned integer value from a local file."
  },
  {
    "title": "ReadUInt32",
    "href": "api/lua/functions.html#readuint32",
    "kind": "function",
    "text": "Read a binary 32 bit unsigned integer value from a local file."
  },
  {
    "title": "ReadUInt8",
    "href": "api/lua/functions.html#readuint8",
    "kind": "function",
    "text": "Read a binary 8 bit unsigned integer value from a local file."
  },
  {
    "title": "ReadVRController",
    "href": "api/lua/functions.html#readvrcontroller",
    "kind": "function",
    "text": "Read the current state of a named VR controller. If no name is passed, `default` is implied. See GetVRControllerNames."
  },
  {
    "title": "ReadVRGenericTracker",
    "href": "api/lua/functions.html#readvrgenerictracker",
    "kind": "function",
    "text": "Read the current state of a named VR generic tracked. If no name is passed, `default` is implied. See GetVRGenericTrackerNames."
  },
  {
    "title": "Reflect",
    "href": "api/lua/functions.html#reflect",
    "kind": "function",
    "text": "Return the input vector reflected around the specified normal."
  },
  {
    "title": "Refract",
    "href": "api/lua/functions.html#refract",
    "kind": "function",
    "text": "Return the input vector refracted around the provided surface normal. - `k_in`: IOR of the medium the vector is exiting. - `k_out`: IOR of the medium the vector is entering."
  },
  {
    "title": "RemoveAssetsFolder",
    "href": "api/lua/functions.html#removeassetsfolder",
    "kind": "function",
    "text": "Remove a folder from the assets system. See man.Assets."
  },
  {
    "title": "RemoveAssetsPackage",
    "href": "api/lua/functions.html#removeassetspackage",
    "kind": "function",
    "text": "Remove a package from the assets system. See man.Assets."
  },
  {
    "title": "RenderInit",
    "href": "api/lua/functions.html#renderinit",
    "kind": "function",
    "text": "Initialize the render system. To change the states of the render system afterward use RenderReset."
  },
  {
    "title": "RenderReset",
    "href": "api/lua/functions.html#renderreset",
    "kind": "function",
    "text": "Change the states of the render system at runtime."
  },
  {
    "title": "RenderResetToWindow",
    "href": "api/lua/functions.html#renderresettowindow",
    "kind": "function",
    "text": "Resize the renderer backbuffer to the provided window client area dimensions. Return true if a reset was needed and carried out."
  },
  {
    "title": "RenderShutdown",
    "href": "api/lua/functions.html#rendershutdown",
    "kind": "function",
    "text": "Shutdown the render system."
  },
  {
    "title": "ResetClock",
    "href": "api/lua/functions.html#resetclock",
    "kind": "function",
    "text": "Reset the elapsed time counter."
  },
  {
    "title": "Reverse",
    "href": "api/lua/functions.html#reverse",
    "kind": "function",
    "text": "Return the provided vector pointing in the opposite direction."
  },
  {
    "title": "ReverseRotationOrder",
    "href": "api/lua/functions.html#reverserotationorder",
    "kind": "function",
    "text": "Return the rotation order processing each axis in the reverse order of the input rotation order."
  },
  {
    "title": "Rewind",
    "href": "api/lua/functions.html#rewind",
    "kind": "function",
    "text": "Rewind the read/write cursor of an open file."
  },
  {
    "title": "RGBA32",
    "href": "api/lua/functions.html#rgba32",
    "kind": "function",
    "text": "Create a 32 bit integer RGBA color."
  },
  {
    "title": "RmDir",
    "href": "api/lua/functions.html#rmdir",
    "kind": "function",
    "text": "Remove an empty folder on the local filesystem. See RmTree."
  },
  {
    "title": "RmTree",
    "href": "api/lua/functions.html#rmtree",
    "kind": "function",
    "text": "Remove a folder on the local filesystem. **Warning:** This function will through all subfolders and erase all files and folders in the target folder."
  },
  {
    "title": "RotationMat2D",
    "href": "api/lua/functions.html#rotationmat2d",
    "kind": "function",
    "text": "Return a 2D rotation matrix by __a__ radians around the specified __pivot__ point."
  },
  {
    "title": "RotationMat3",
    "href": "api/lua/functions.html#rotationmat3",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix."
  },
  {
    "title": "RotationMat4",
    "href": "api/lua/functions.html#rotationmat4",
    "kind": "function",
    "text": "Return a 4x3 rotation matrix from euler angles. The default rotation order is YXZ."
  },
  {
    "title": "RotationMatX",
    "href": "api/lua/functions.html#rotationmatx",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the world X axis {1, 0, 0}."
  },
  {
    "title": "RotationMatXY",
    "href": "api/lua/functions.html#rotationmatxy",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the X axis followed by a rotation around the Y axis."
  },
  {
    "title": "RotationMatXYZ",
    "href": "api/lua/functions.html#rotationmatxyz",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the X axis followed by a rotation around the Y axis then a rotation around the Z axis."
  },
  {
    "title": "RotationMatXZY",
    "href": "api/lua/functions.html#rotationmatxzy",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the X axis followed by a rotation around the Z axis then a rotation around the Y axis."
  },
  {
    "title": "RotationMatY",
    "href": "api/lua/functions.html#rotationmaty",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the world Y axis {0, 1, 0}."
  },
  {
    "title": "RotationMatYXZ",
    "href": "api/lua/functions.html#rotationmatyxz",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the Y axis followed by a rotation around the X axis then a rotation around the Z axis."
  },
  {
    "title": "RotationMatYZX",
    "href": "api/lua/functions.html#rotationmatyzx",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the Y axis followed by a rotation around the Z axis then a rotation around the X axis."
  },
  {
    "title": "RotationMatZ",
    "href": "api/lua/functions.html#rotationmatz",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the world Z axis {0, 0, 1}."
  },
  {
    "title": "RotationMatZXY",
    "href": "api/lua/functions.html#rotationmatzxy",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the Z axis followed by a rotation around the X axis then a rotation around the Y axis."
  },
  {
    "title": "RotationMatZYX",
    "href": "api/lua/functions.html#rotationmatzyx",
    "kind": "function",
    "text": "Return a 3x3 rotation matrix around the Z axis followed by a rotation around the Y axis then a rotation around the X axis."
  },
  {
    "title": "SaveBMP",
    "href": "api/lua/functions.html#savebmp",
    "kind": "function",
    "text": "Save a Picture in [BMP](https://en.wikipedia.org/wiki/BMP_file_format) file format."
  },
  {
    "title": "SaveDataToFile",
    "href": "api/lua/functions.html#savedatatofile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveFileDialog",
    "href": "api/lua/functions.html#savefiledialog",
    "kind": "function",
    "text": "Open a native SaveFile dialog."
  },
  {
    "title": "SaveForwardPipelineAAAConfigToFile",
    "href": "api/lua/functions.html#saveforwardpipelineaaaconfigtofile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveGeometryToFile",
    "href": "api/lua/functions.html#savegeometrytofile",
    "kind": "function",
    "text": "Save a geometry to the local filesystem. Note that in order to render a geometry it must have been converted to model by the asset compiler. See GeometryBuilder and ModelBuilder."
  },
  {
    "title": "SaveJsonToFile",
    "href": "api/lua/functions.html#savejsontofile",
    "kind": "function",
    "text": "Save a JSON object to the local filesystem."
  },
  {
    "title": "SavePNG",
    "href": "api/lua/functions.html#savepng",
    "kind": "function",
    "text": "Save a Picture in [PNG](https://en.wikipedia.org/wiki/Portable_Network_Graphics) file format."
  },
  {
    "title": "SaveSceneBinaryToData",
    "href": "api/lua/functions.html#savescenebinarytodata",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveSceneBinaryToFile",
    "href": "api/lua/functions.html#savescenebinarytofile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveSceneJsonToFile",
    "href": "api/lua/functions.html#savescenejsontofile",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SaveTGA",
    "href": "api/lua/functions.html#savetga",
    "kind": "function",
    "text": "Save a Picture in [TGA](https://en.wikipedia.org/wiki/Truevision_TGA) file format."
  },
  {
    "title": "ScaleMat3",
    "href": "api/lua/functions.html#scalemat3",
    "kind": "function",
    "text": "Return a 3x3 scale matrix from a 2D vector."
  },
  {
    "title": "ScaleMat4",
    "href": "api/lua/functions.html#scalemat4",
    "kind": "function",
    "text": "Return a 4x3 scale matrix from the parameter scaling vector."
  },
  {
    "title": "SceneClearSystems",
    "href": "api/lua/functions.html#sceneclearsystems",
    "kind": "function",
    "text": "Clear scene and all optional systems."
  },
  {
    "title": "SceneGarbageCollectSystems",
    "href": "api/lua/functions.html#scenegarbagecollectsystems",
    "kind": "function",
    "text": "Garbage collect a scene and all its optional systems."
  },
  {
    "title": "SceneSyncToSystemsFromAssets",
    "href": "api/lua/functions.html#scenesynctosystemsfromassets",
    "kind": "function",
    "text": "Synchronize optional systems (eg. physics or script) states with the scene states. Load resources from the assets system if required. See man.Assets."
  },
  {
    "title": "SceneSyncToSystemsFromFile",
    "href": "api/lua/functions.html#scenesynctosystemsfromfile",
    "kind": "function",
    "text": "Synchronize optional systems (eg. physics or script) states with the scene states. Load resources from the local filesystem if required. See man.Assets."
  },
  {
    "title": "SceneUpdateSystems",
    "href": "api/lua/functions.html#sceneupdatesystems",
    "kind": "function",
    "text": "Update a scene and all its optional systems."
  },
  {
    "title": "ScreenSpaceToClipSpace",
    "href": "api/lua/functions.html#screenspacetoclipspace",
    "kind": "function",
    "text": "Transform a screen position to clip space."
  },
  {
    "title": "Sec",
    "href": "api/lua/functions.html#sec",
    "kind": "function",
    "text": "Convert a value in seconds to the Harfang internal unit system. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "Seed",
    "href": "api/lua/functions.html#seed",
    "kind": "function",
    "text": "Set the starting seed of the pseudo-random number generator."
  },
  {
    "title": "Seek",
    "href": "api/lua/functions.html#seek",
    "kind": "function",
    "text": "Move the handle cursor to a specific position in the file."
  },
  {
    "title": "SendVRControllerHapticPulse",
    "href": "api/lua/functions.html#sendvrcontrollerhapticpulse",
    "kind": "function",
    "text": "Send an haptic pulse to a named VR controller. See GetVRControllerNames."
  },
  {
    "title": "SetAxises",
    "href": "api/lua/functions.html#setaxises",
    "kind": "function",
    "text": "Inject X, Y and Z axises into a 3x3 matrix."
  },
  {
    "title": "SetColumn",
    "href": "api/lua/functions.html#setcolumn",
    "kind": "function",
    "text": "Returns the nth column."
  },
  {
    "title": "SetHeight",
    "href": "api/lua/functions.html#setheight",
    "kind": "function",
    "text": "Set a rectangle height."
  },
  {
    "title": "SetJsonValue",
    "href": "api/lua/functions.html#setjsonvalue",
    "kind": "function",
    "text": "Set a JSON key value."
  },
  {
    "title": "SetListener",
    "href": "api/lua/functions.html#setlistener",
    "kind": "function",
    "text": "Set the listener transformation and velocity for spatialization by the audio system."
  },
  {
    "title": "SetLogDetailed",
    "href": "api/lua/functions.html#setlogdetailed",
    "kind": "function",
    "text": "Display the `details` field of log outputs."
  },
  {
    "title": "SetLogLevel",
    "href": "api/lua/functions.html#setloglevel",
    "kind": "function",
    "text": "Control which log levels should be displayed. See Log, Warn, Error and Debug."
  },
  {
    "title": "SetMaterialAlphaCut",
    "href": "api/lua/functions.html#setmaterialalphacut",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialAmbientUsesUV1",
    "href": "api/lua/functions.html#setmaterialambientusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialBlendMode",
    "href": "api/lua/functions.html#setmaterialblendmode",
    "kind": "function",
    "text": "Set material blend mode."
  },
  {
    "title": "SetMaterialDepthTest",
    "href": "api/lua/functions.html#setmaterialdepthtest",
    "kind": "function",
    "text": "Set material depth test."
  },
  {
    "title": "SetMaterialDiffuseUsesUV1",
    "href": "api/lua/functions.html#setmaterialdiffuseusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialFaceCulling",
    "href": "api/lua/functions.html#setmaterialfaceculling",
    "kind": "function",
    "text": "Set material face culling."
  },
  {
    "title": "SetMaterialNormalMapInWorldSpace",
    "href": "api/lua/functions.html#setmaterialnormalmapinworldspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialProgram",
    "href": "api/lua/functions.html#setmaterialprogram",
    "kind": "function",
    "text": "Set material pipeline program. You should call UpdateMaterialPipelineProgramVariant after changing a material pipeline program so that the correct variant is selected according to the material states."
  },
  {
    "title": "SetMaterialSkinning",
    "href": "api/lua/functions.html#setmaterialskinning",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialSpecularUsesUV1",
    "href": "api/lua/functions.html#setmaterialspecularusesuv1",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetMaterialTexture",
    "href": "api/lua/functions.html#setmaterialtexture",
    "kind": "function",
    "text": "Set a material uniform texture and texture stage. Note: The texture stage specified should match the uniform declaration in the shader program."
  },
  {
    "title": "SetMaterialTextureRef",
    "href": "api/lua/functions.html#setmaterialtextureref",
    "kind": "function",
    "text": "Set a material uniform texture reference. See PipelineResources."
  },
  {
    "title": "SetMaterialValue",
    "href": "api/lua/functions.html#setmaterialvalue",
    "kind": "function",
    "text": "Set a material uniform value."
  },
  {
    "title": "SetMaterialWriteRGBA",
    "href": "api/lua/functions.html#setmaterialwritergba",
    "kind": "function",
    "text": "Set a material color write mask."
  },
  {
    "title": "SetMaterialWriteZ",
    "href": "api/lua/functions.html#setmaterialwritez",
    "kind": "function",
    "text": "Set a material depth write mask."
  },
  {
    "title": "SetRenderDebug",
    "href": "api/lua/functions.html#setrenderdebug",
    "kind": "function",
    "text": "Set render system debug flags."
  },
  {
    "title": "SetRow",
    "href": "api/lua/functions.html#setrow",
    "kind": "function",
    "text": "Sets the nth row of a matrix."
  },
  {
    "title": "SetS",
    "href": "api/lua/functions.html#sets",
    "kind": "function",
    "text": "Shortcut for SetScale."
  },
  {
    "title": "SetSaturation",
    "href": "api/lua/functions.html#setsaturation",
    "kind": "function",
    "text": "Return a copy of the input RGBA color with its saturation set to the specified value, alpha channel is left unmodified. See ToHLS and FromHLS."
  },
  {
    "title": "SetScale",
    "href": "api/lua/functions.html#setscale",
    "kind": "function",
    "text": "Set the scaling part of the transformation matrix."
  },
  {
    "title": "SetSourcePanning",
    "href": "api/lua/functions.html#setsourcepanning",
    "kind": "function",
    "text": "Set a playing audio source panning."
  },
  {
    "title": "SetSourceRepeat",
    "href": "api/lua/functions.html#setsourcerepeat",
    "kind": "function",
    "text": "Set audio source repeat mode."
  },
  {
    "title": "SetSourceTimecode",
    "href": "api/lua/functions.html#setsourcetimecode",
    "kind": "function",
    "text": "Set timecode of the audio source."
  },
  {
    "title": "SetSourceTransform",
    "href": "api/lua/functions.html#setsourcetransform",
    "kind": "function",
    "text": "Set a playing spatialized audio source transformation."
  },
  {
    "title": "SetSourceVolume",
    "href": "api/lua/functions.html#setsourcevolume",
    "kind": "function",
    "text": "Set audio source volume."
  },
  {
    "title": "SetT",
    "href": "api/lua/functions.html#sett",
    "kind": "function",
    "text": "Shortcut for SetTranslation."
  },
  {
    "title": "SetTransform",
    "href": "api/lua/functions.html#settransform",
    "kind": "function",
    "text": "Set the model matrix for the next drawn primitive. If not called, model will be rendered with the identity model matrix."
  },
  {
    "title": "SetTranslation",
    "href": "api/lua/functions.html#settranslation",
    "kind": "function",
    "text": "Sets the 2D translation part, i.e. the first 2 elements of the last matrix row."
  },
  {
    "title": "SetView2D",
    "href": "api/lua/functions.html#setview2d",
    "kind": "function",
    "text": "High-level wrapper function to setup a view for 2D rendering. This function calls SetViewClear, SetViewRect then SetViewTransform."
  },
  {
    "title": "SetViewClear",
    "href": "api/lua/functions.html#setviewclear",
    "kind": "function",
    "text": "Set a view clear parameters. See man.Views."
  },
  {
    "title": "SetViewFrameBuffer",
    "href": "api/lua/functions.html#setviewframebuffer",
    "kind": "function",
    "text": "Set view output framebuffer. See man.Views."
  },
  {
    "title": "SetViewMode",
    "href": "api/lua/functions.html#setviewmode",
    "kind": "function",
    "text": "Set view draw ordering mode."
  },
  {
    "title": "SetViewOrthographic",
    "href": "api/lua/functions.html#setvieworthographic",
    "kind": "function",
    "text": "High-level wrapper function to setup a view for 3D orthographic rendering. This function calls SetViewClear, SetViewRect then SetViewTransform."
  },
  {
    "title": "SetViewPerspective",
    "href": "api/lua/functions.html#setviewperspective",
    "kind": "function",
    "text": "High-level wrapper function to setup a view for 3D perspective rendering. This function calls SetViewClear, SetViewRect then SetViewTransform."
  },
  {
    "title": "SetViewRect",
    "href": "api/lua/functions.html#setviewrect",
    "kind": "function",
    "text": ""
  },
  {
    "title": "SetViewTransform",
    "href": "api/lua/functions.html#setviewtransform",
    "kind": "function",
    "text": "Set view transforms, namely the view and projection matrices."
  },
  {
    "title": "SetWidth",
    "href": "api/lua/functions.html#setwidth",
    "kind": "function",
    "text": "Set a rectangle width."
  },
  {
    "title": "SetWindowClientSize",
    "href": "api/lua/functions.html#setwindowclientsize",
    "kind": "function",
    "text": "Set the window client size. The client area of a window excludes its decoration."
  },
  {
    "title": "SetWindowPos",
    "href": "api/lua/functions.html#setwindowpos",
    "kind": "function",
    "text": "Set window position."
  },
  {
    "title": "SetWindowTitle",
    "href": "api/lua/functions.html#setwindowtitle",
    "kind": "function",
    "text": "Set window title."
  },
  {
    "title": "SetX",
    "href": "api/lua/functions.html#setx",
    "kind": "function",
    "text": "Sets the first row."
  },
  {
    "title": "SetY",
    "href": "api/lua/functions.html#sety",
    "kind": "function",
    "text": "Sets the second row."
  },
  {
    "title": "SetZ",
    "href": "api/lua/functions.html#setz",
    "kind": "function",
    "text": "Sets the third row."
  },
  {
    "title": "ShowCursor",
    "href": "api/lua/functions.html#showcursor",
    "kind": "function",
    "text": "Show the system mouse cursor. See HideCursor."
  },
  {
    "title": "Sign",
    "href": "api/lua/functions.html#sign",
    "kind": "function",
    "text": "Returns a vector whose elements are -1 if the corresponding vector element is = 0."
  },
  {
    "title": "SkipClock",
    "href": "api/lua/functions.html#skipclock",
    "kind": "function",
    "text": "Skip elapsed time since the last call to TickClock."
  },
  {
    "title": "Sleep",
    "href": "api/lua/functions.html#sleep",
    "kind": "function",
    "text": "Sleep the caller thread, this function will resume execution after waiting for at least the specified amount of time."
  },
  {
    "title": "Slerp",
    "href": "api/lua/functions.html#slerp",
    "kind": "function",
    "text": "Interpolate between the rotation represented by two quaternions. The _Spherical Linear Interpolation_ will always take the shortest path between the two rotations."
  },
  {
    "title": "SRanipalGetState",
    "href": "api/lua/functions.html#sranipalgetstate",
    "kind": "function",
    "text": "Return the current SRanipal device state."
  },
  {
    "title": "SRanipalInit",
    "href": "api/lua/functions.html#sranipalinit",
    "kind": "function",
    "text": "Initial the SRanipal eye detection SDK."
  },
  {
    "title": "SRanipalIsViveProEye",
    "href": "api/lua/functions.html#sranipalisviveproeye",
    "kind": "function",
    "text": "Return `true` if the eye detection device in use is Vive Pro Eye."
  },
  {
    "title": "SRanipalLaunchEyeCalibration",
    "href": "api/lua/functions.html#sranipallauncheyecalibration",
    "kind": "function",
    "text": "Launch the eye detection calibration sequence."
  },
  {
    "title": "SRanipalShutdown",
    "href": "api/lua/functions.html#sranipalshutdown",
    "kind": "function",
    "text": "Shutdown the SRanipal eye detection SDK."
  },
  {
    "title": "StopAllSources",
    "href": "api/lua/functions.html#stopallsources",
    "kind": "function",
    "text": "Stop all playing audio sources."
  },
  {
    "title": "StopSource",
    "href": "api/lua/functions.html#stopsource",
    "kind": "function",
    "text": "Stop a playing audio source."
  },
  {
    "title": "StreamOGGAssetSpatialized",
    "href": "api/lua/functions.html#streamoggassetspatialized",
    "kind": "function",
    "text": ""
  },
  {
    "title": "StreamOGGAssetStereo",
    "href": "api/lua/functions.html#streamoggassetstereo",
    "kind": "function",
    "text": ""
  },
  {
    "title": "StreamOGGFileSpatialized",
    "href": "api/lua/functions.html#streamoggfilespatialized",
    "kind": "function",
    "text": ""
  },
  {
    "title": "StreamOGGFileStereo",
    "href": "api/lua/functions.html#streamoggfilestereo",
    "kind": "function",
    "text": ""
  },
  {
    "title": "StreamWAVAssetSpatialized",
    "href": "api/lua/functions.html#streamwavassetspatialized",
    "kind": "function",
    "text": "Start an audio stream from a WAV file from the assets system. See SetSourceTransform and man.Assets."
  },
  {
    "title": "StreamWAVAssetStereo",
    "href": "api/lua/functions.html#streamwavassetstereo",
    "kind": "function",
    "text": "Start an audio stream from a WAV file from the assets system. See man.Assets."
  },
  {
    "title": "StreamWAVFileSpatialized",
    "href": "api/lua/functions.html#streamwavfilespatialized",
    "kind": "function",
    "text": "Start an audio stream from a WAV file on the local filesystem. See SetSourceTransform."
  },
  {
    "title": "StreamWAVFileStereo",
    "href": "api/lua/functions.html#streamwavfilestereo",
    "kind": "function",
    "text": "Start an audio stream from a WAV file on the local filesystem. See man.Assets."
  },
  {
    "title": "StringToFile",
    "href": "api/lua/functions.html#stringtofile",
    "kind": "function",
    "text": "Return the content of a file on the local filesystem as a string."
  },
  {
    "title": "SubmitSceneToForwardPipeline",
    "href": "api/lua/functions.html#submitscenetoforwardpipeline",
    "kind": "function",
    "text": "Submit a scene to a forward pipeline. See PrepareSceneForwardPipelineCommonRenderData and PrepareSceneForwardPipelineViewDependentRenderData if you need to render the same scene from different points of view."
  },
  {
    "title": "SubmitSceneToPipeline",
    "href": "api/lua/functions.html#submitscenetopipeline",
    "kind": "function",
    "text": "See SubmitSceneToForwardPipeline."
  },
  {
    "title": "SwapFileExtension",
    "href": "api/lua/functions.html#swapfileextension",
    "kind": "function",
    "text": "Return the input file path with its extension replaced."
  },
  {
    "title": "Tell",
    "href": "api/lua/functions.html#tell",
    "kind": "function",
    "text": "Return the current handle cursor position in bytes."
  },
  {
    "title": "TestVisibility",
    "href": "api/lua/functions.html#testvisibility",
    "kind": "function",
    "text": "Test if a list of 3d points are inside or outside a Frustum."
  },
  {
    "title": "TickClock",
    "href": "api/lua/functions.html#tickclock",
    "kind": "function",
    "text": "Advance the engine clock and return the elapsed time since the last call to this function. See GetClock to retrieve the current clock. See GetClockDt."
  },
  {
    "title": "time_from_day",
    "href": "api/lua/functions.html#time_from_day",
    "kind": "function",
    "text": "Convert days to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_hour",
    "href": "api/lua/functions.html#time_from_hour",
    "kind": "function",
    "text": "Convert hours to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_min",
    "href": "api/lua/functions.html#time_from_min",
    "kind": "function",
    "text": "Convert minutes to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_ms",
    "href": "api/lua/functions.html#time_from_ms",
    "kind": "function",
    "text": "Convert milliseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_ms_f",
    "href": "api/lua/functions.html#time_from_ms_f",
    "kind": "function",
    "text": "Convert milliseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_ns",
    "href": "api/lua/functions.html#time_from_ns",
    "kind": "function",
    "text": "Convert nanoseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_sec",
    "href": "api/lua/functions.html#time_from_sec",
    "kind": "function",
    "text": "Convert seconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_sec_f",
    "href": "api/lua/functions.html#time_from_sec_f",
    "kind": "function",
    "text": "Convert fractional seconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_us",
    "href": "api/lua/functions.html#time_from_us",
    "kind": "function",
    "text": "Convert microseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_from_us_f",
    "href": "api/lua/functions.html#time_from_us_f",
    "kind": "function",
    "text": "Convert fractional microseconds to time. See man.CoordinateAndUnitSystem."
  },
  {
    "title": "time_now",
    "href": "api/lua/functions.html#time_now",
    "kind": "function",
    "text": "Return the current system time."
  },
  {
    "title": "time_to_day",
    "href": "api/lua/functions.html#time_to_day",
    "kind": "function",
    "text": "Convert time to days."
  },
  {
    "title": "time_to_hour",
    "href": "api/lua/functions.html#time_to_hour",
    "kind": "function",
    "text": "Convert time to hours."
  },
  {
    "title": "time_to_min",
    "href": "api/lua/functions.html#time_to_min",
    "kind": "function",
    "text": "Convert time to minutes."
  },
  {
    "title": "time_to_ms",
    "href": "api/lua/functions.html#time_to_ms",
    "kind": "function",
    "text": "Convert time to milliseconds."
  },
  {
    "title": "time_to_ms_f",
    "href": "api/lua/functions.html#time_to_ms_f",
    "kind": "function",
    "text": "Convert time to miliseconds."
  },
  {
    "title": "time_to_ns",
    "href": "api/lua/functions.html#time_to_ns",
    "kind": "function",
    "text": "Convert time to nanoseconds."
  },
  {
    "title": "time_to_sec",
    "href": "api/lua/functions.html#time_to_sec",
    "kind": "function",
    "text": "Convert time to seconds."
  },
  {
    "title": "time_to_sec_f",
    "href": "api/lua/functions.html#time_to_sec_f",
    "kind": "function",
    "text": "Convert time to fractional seconds."
  },
  {
    "title": "time_to_string",
    "href": "api/lua/functions.html#time_to_string",
    "kind": "function",
    "text": "Return time as a human-readable string."
  },
  {
    "title": "time_to_us",
    "href": "api/lua/functions.html#time_to_us",
    "kind": "function",
    "text": "Convert time to microseconds."
  },
  {
    "title": "time_to_us_f",
    "href": "api/lua/functions.html#time_to_us_f",
    "kind": "function",
    "text": "Convert time to fractional microseconds."
  },
  {
    "title": "ToEuler",
    "href": "api/lua/functions.html#toeuler",
    "kind": "function",
    "text": "Convert a quaternion rotation to its _Euler_ vector representation."
  },
  {
    "title": "ToFloatRect",
    "href": "api/lua/functions.html#tofloatrect",
    "kind": "function",
    "text": "Return an integer rectangle as a floating point rectangle."
  },
  {
    "title": "ToHLS",
    "href": "api/lua/functions.html#tohls",
    "kind": "function",
    "text": "Convert input RGBA color to hue/luminance/saturation, alpha channel is left unmodified."
  },
  {
    "title": "ToIntRect",
    "href": "api/lua/functions.html#tointrect",
    "kind": "function",
    "text": "Return a floating point rectangle as an integer rectangle."
  },
  {
    "title": "ToMatrix3",
    "href": "api/lua/functions.html#tomatrix3",
    "kind": "function",
    "text": "Convert a quaternion rotation to its Mat3 representation."
  },
  {
    "title": "Touch",
    "href": "api/lua/functions.html#touch",
    "kind": "function",
    "text": "Submit an empty primitive to the view. See Frame."
  },
  {
    "title": "TransformationMat4",
    "href": "api/lua/functions.html#transformationmat4",
    "kind": "function",
    "text": "Creates a 4x3 transformation matrix from the translation vector __p__, the 3x3 rotation Matrix __m__ (or YXZ euler rotation vector __e__) and the scaling vector __s__. This is a more efficient version of `TranslationMat4(p) * ScaleMat4(s) *"
  },
  {
    "title": "TransformFrustum",
    "href": "api/lua/functions.html#transformfrustum",
    "kind": "function",
    "text": "Return the input frustum transformed by the provided world matrix."
  },
  {
    "title": "TranslationMat3",
    "href": "api/lua/functions.html#translationmat3",
    "kind": "function",
    "text": "Return a 2D translation 3x3 matrix from the first 2 components (__x__,__y__) of the parameter vector."
  },
  {
    "title": "TranslationMat4",
    "href": "api/lua/functions.html#translationmat4",
    "kind": "function",
    "text": "Return a 4x3 translation matrix from the parameter displacement vector."
  },
  {
    "title": "Transpose",
    "href": "api/lua/functions.html#transpose",
    "kind": "function",
    "text": "Return the transpose of the input matrix. For a pure rotation matrix this returns the opposite transformation so that M*M T =I."
  },
  {
    "title": "Union",
    "href": "api/lua/functions.html#union",
    "kind": "function",
    "text": "Compute the union of this bounding volume with another volume or a 3d position."
  },
  {
    "title": "Unlink",
    "href": "api/lua/functions.html#unlink",
    "kind": "function",
    "text": "Remove a file from the local filesystem."
  },
  {
    "title": "UnloadSound",
    "href": "api/lua/functions.html#unloadsound",
    "kind": "function",
    "text": "Unload a sound from the audio system."
  },
  {
    "title": "UnprojectFromClipSpace",
    "href": "api/lua/functions.html#unprojectfromclipspace",
    "kind": "function",
    "text": "Unproject a clip space position to view space."
  },
  {
    "title": "UnprojectFromScreenSpace",
    "href": "api/lua/functions.html#unprojectfromscreenspace",
    "kind": "function",
    "text": "Unproject a screen space position to view space."
  },
  {
    "title": "UnprojectOrthoFromClipSpace",
    "href": "api/lua/functions.html#unprojectorthofromclipspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "UnprojectOrthoFromScreenSpace",
    "href": "api/lua/functions.html#unprojectorthofromscreenspace",
    "kind": "function",
    "text": ""
  },
  {
    "title": "UpdateMaterialPipelineProgramVariant",
    "href": "api/lua/functions.html#updatematerialpipelineprogramvariant",
    "kind": "function",
    "text": "Select the proper pipeline program variant for the current material state."
  },
  {
    "title": "UpdateTexture",
    "href": "api/lua/functions.html#updatetexture",
    "kind": "function",
    "text": ""
  },
  {
    "title": "UpdateTextureFromPicture",
    "href": "api/lua/functions.html#updatetexturefrompicture",
    "kind": "function",
    "text": "Update texture content from the provided picture. Note: The picture is expected to be in a format compatible with the texture format."
  },
  {
    "title": "UpdateWindow",
    "href": "api/lua/functions.html#updatewindow",
    "kind": "function",
    "text": "Update a window on the host system."
  },
  {
    "title": "Vec3I",
    "href": "api/lua/functions.html#vec3i",
    "kind": "function",
    "text": "Create a vector from integer values in the [0;255] range."
  },
  {
    "title": "Vec4I",
    "href": "api/lua/functions.html#vec4i",
    "kind": "function",
    "text": "Create a vector from integer values in the [0;255] range."
  },
  {
    "title": "VectorMat3",
    "href": "api/lua/functions.html#vectormat3",
    "kind": "function",
    "text": "Return a vector as a matrix."
  },
  {
    "title": "VertexLayoutPosFloatColorFloat",
    "href": "api/lua/functions.html#vertexlayoutposfloatcolorfloat",
    "kind": "function",
    "text": ""
  },
  {
    "title": "VertexLayoutPosFloatColorUInt8",
    "href": "api/lua/functions.html#vertexlayoutposfloatcoloruint8",
    "kind": "function",
    "text": ""
  },
  {
    "title": "VertexLayoutPosFloatNormFloat",
    "href": "api/lua/functions.html#vertexlayoutposfloatnormfloat",
    "kind": "function",
    "text": "Simple vertex layout with float position and normal. ```python vtx_layout = VertexLayout() vtx_layout.Begin() vtx_layout.Add(hg.A_Position, 3, hg.AT_Float) vtx_layout.Add(hg.A_Normal, 3, hg.AT_Float) vtx_layout.End() ```"
  },
  {
    "title": "VertexLayoutPosFloatNormUInt8",
    "href": "api/lua/functions.html#vertexlayoutposfloatnormuint8",
    "kind": "function",
    "text": "Simple vertex layout with float position and 8-bit unsigned integer normal. ```python vtx_layout = VertexLayout() vtx_layout.Begin() vtx_layout.Add(hg.A_Position, 3, hg.AT_Float) vtx_layout.Add(hg.A_Normal, 3, hg.AT_Uint8, True, True) vtx_l"
  },
  {
    "title": "VertexLayoutPosFloatNormUInt8TexCoord0UInt8",
    "href": "api/lua/functions.html#vertexlayoutposfloatnormuint8texcoord0uint8",
    "kind": "function",
    "text": ""
  },
  {
    "title": "VertexLayoutPosFloatTexCoord0UInt8",
    "href": "api/lua/functions.html#vertexlayoutposfloattexcoord0uint8",
    "kind": "function",
    "text": ""
  },
  {
    "title": "Warn",
    "href": "api/lua/functions.html#warn",
    "kind": "function",
    "text": ""
  },
  {
    "title": "WindowHasFocus",
    "href": "api/lua/functions.html#windowhasfocus",
    "kind": "function",
    "text": "Return `true` if the provided window has focus, `false` otherwise."
  },
  {
    "title": "WindowSystemInit",
    "href": "api/lua/functions.html#windowsysteminit",
    "kind": "function",
    "text": "Initialize the Window system."
  },
  {
    "title": "WindowSystemShutdown",
    "href": "api/lua/functions.html#windowsystemshutdown",
    "kind": "function",
    "text": "Shutdown the window system. See WindowSystemInit."
  },
  {
    "title": "Wrap",
    "href": "api/lua/functions.html#wrap",
    "kind": "function",
    "text": "Wrap the input value so that it fits in the specified inclusive range."
  },
  {
    "title": "WriteFloat",
    "href": "api/lua/functions.html#writefloat",
    "kind": "function",
    "text": "Write a binary 32 bit floating point value to a file."
  },
  {
    "title": "WriteString",
    "href": "api/lua/functions.html#writestring",
    "kind": "function",
    "text": "Write a string to a file as 32 bit integer size followed by the string content in UTF8."
  },
  {
    "title": "WriteUInt16",
    "href": "api/lua/functions.html#writeuint16",
    "kind": "function",
    "text": "Write a binary 16 bit unsigned integer to a file."
  },
  {
    "title": "WriteUInt32",
    "href": "api/lua/functions.html#writeuint32",
    "kind": "function",
    "text": "Write a binary 32 bit unsigned integer to a file."
  },
  {
    "title": "WriteUInt8",
    "href": "api/lua/functions.html#writeuint8",
    "kind": "function",
    "text": "Write a binary 8 bit unsigned integer to a file."
  },
  {
    "title": "ZoomFactorToFov",
    "href": "api/lua/functions.html#zoomfactortofov",
    "kind": "function",
    "text": "Convert from a zoom factor value in meters to a fov value in radian."
  },
  {
    "title": "AnimLoopMode",
    "href": "api/lua/constants.html#animloopmode",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Attrib",
    "href": "api/lua/constants.html#attrib",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "AttribType",
    "href": "api/lua/constants.html#attribtype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "AudioFrameFormat",
    "href": "api/lua/constants.html#audioframeformat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Axis",
    "href": "api/lua/constants.html#axis",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "BackbufferRatio",
    "href": "api/lua/constants.html#backbufferratio",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "BlendMode",
    "href": "api/lua/constants.html#blendmode",
    "kind": "enumeration",
    "text": "Control the compositing mode used to draw primitives."
  },
  {
    "title": "CollisionEventTrackingMode",
    "href": "api/lua/constants.html#collisioneventtrackingmode",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "CollisionType",
    "href": "api/lua/constants.html#collisiontype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "DepthTest",
    "href": "api/lua/constants.html#depthtest",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "DirEntryType",
    "href": "api/lua/constants.html#direntrytype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "DrawTextHAlign",
    "href": "api/lua/constants.html#drawtexthalign",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "DrawTextVAlign",
    "href": "api/lua/constants.html#drawtextvalign",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Easing",
    "href": "api/lua/constants.html#easing",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "FaceCulling",
    "href": "api/lua/constants.html#faceculling",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ForwardPipelineAAADebugBuffer",
    "href": "api/lua/constants.html#forwardpipelineaaadebugbuffer",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ForwardPipelineLightType",
    "href": "api/lua/constants.html#forwardpipelinelighttype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ForwardPipelineShadowType",
    "href": "api/lua/constants.html#forwardpipelineshadowtype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "GamepadAxes",
    "href": "api/lua/constants.html#gamepadaxes",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "GamepadButton",
    "href": "api/lua/constants.html#gamepadbutton",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "HandsSide",
    "href": "api/lua/constants.html#handsside",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImDrawFlags",
    "href": "api/lua/constants.html#imdrawflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiCol",
    "href": "api/lua/constants.html#imguicol",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiColorEditFlags",
    "href": "api/lua/constants.html#imguicoloreditflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiComboFlags",
    "href": "api/lua/constants.html#imguicomboflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiCond",
    "href": "api/lua/constants.html#imguicond",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiFocusedFlags",
    "href": "api/lua/constants.html#imguifocusedflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiHoveredFlags",
    "href": "api/lua/constants.html#imguihoveredflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiInputTextFlags",
    "href": "api/lua/constants.html#imguiinputtextflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiMouseButton",
    "href": "api/lua/constants.html#imguimousebutton",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiPopupFlags",
    "href": "api/lua/constants.html#imguipopupflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiSelectableFlags",
    "href": "api/lua/constants.html#imguiselectableflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiStyleVar",
    "href": "api/lua/constants.html#imguistylevar",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiTreeNodeFlags",
    "href": "api/lua/constants.html#imguitreenodeflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ImGuiWindowFlags",
    "href": "api/lua/constants.html#imguiwindowflags",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Key",
    "href": "api/lua/constants.html#key",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "LightShadowType",
    "href": "api/lua/constants.html#lightshadowtype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "LightType",
    "href": "api/lua/constants.html#lighttype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "LogLevel",
    "href": "api/lua/constants.html#loglevel",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "MonitorRotation",
    "href": "api/lua/constants.html#monitorrotation",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "MouseButton",
    "href": "api/lua/constants.html#mousebutton",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "NodeComponentIdx",
    "href": "api/lua/constants.html#nodecomponentidx",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "OpenVRAA",
    "href": "api/lua/constants.html#openvraa",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "OpenXRAA",
    "href": "api/lua/constants.html#openxraa",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "OpenXRExtensions",
    "href": "api/lua/constants.html#openxrextensions",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "PictureFormat",
    "href": "api/lua/constants.html#pictureformat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "RendererType",
    "href": "api/lua/constants.html#renderertype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "RigidBodyType",
    "href": "api/lua/constants.html#rigidbodytype",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "RotationOrder",
    "href": "api/lua/constants.html#rotationorder",
    "kind": "enumeration",
    "text": "This enumeration is used to control the order of rotation around the X, Y and Z axises."
  },
  {
    "title": "SceneForwardPipelinePass",
    "href": "api/lua/constants.html#sceneforwardpipelinepass",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "SeekMode",
    "href": "api/lua/constants.html#seekmode",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "SourceRepeat",
    "href": "api/lua/constants.html#sourcerepeat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "SourceState",
    "href": "api/lua/constants.html#sourcestate",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "TextureFormat",
    "href": "api/lua/constants.html#textureformat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "VideoFrameFormat",
    "href": "api/lua/constants.html#videoframeformat",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ViewMode",
    "href": "api/lua/constants.html#viewmode",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "Visibility",
    "href": "api/lua/constants.html#visibility",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "VRControllerButton",
    "href": "api/lua/constants.html#vrcontrollerbutton",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "WindowVisibility",
    "href": "api/lua/constants.html#windowvisibility",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "XrHandJoint",
    "href": "api/lua/constants.html#xrhandjoint",
    "kind": "enumeration",
    "text": ""
  },
  {
    "title": "ClearFlags",
    "href": "api/lua/constants.html#clearflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "DebugFlags",
    "href": "api/lua/constants.html#debugflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "LoadSaveSceneFlags",
    "href": "api/lua/constants.html#loadsavesceneflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "ResetFlags",
    "href": "api/lua/constants.html#resetflags",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "SoundRef",
    "href": "api/lua/constants.html#soundref",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "SourceRef",
    "href": "api/lua/constants.html#sourceref",
    "kind": "constants",
    "text": ""
  },
  {
    "title": "TextureFlags",
    "href": "api/lua/constants.html#textureflags",
    "kind": "constants",
    "text": ""
  }
];

(function () {
	function escapeHtml(value) {
		return String(value).replace(/[&<>"']/g, function (ch) {
			return ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[ch];
		});
	}

	function installSearch() {
		var input = document.getElementById("doc-search");
		var results = document.getElementById("doc-search-results");
		if (!input || !results) {
			return;
		}

		var root = document.body.getAttribute("data-root") || "";
		var index = window.HG_SEARCH_INDEX || [];

		input.addEventListener("input", function () {
			var query = input.value.trim().toLowerCase();
			if (query.length < 2) {
				results.className = "doc-search-results";
				results.innerHTML = "";
				return;
			}

			var matches = index.filter(function (item) {
				return (item.title + " " + item.kind + " " + item.text).toLowerCase().indexOf(query) !== -1;
			}).slice(0, 20);

			if (!matches.length) {
				results.className = "doc-search-results active";
				results.innerHTML = "<small>No result.</small>";
				return;
			}

			results.className = "doc-search-results active";
			results.innerHTML = matches.map(function (item) {
				return '<a class="doc-search-result" href="' + root + item.href + '">' +
					escapeHtml(item.title) + '<small>' + escapeHtml(item.kind) + '</small></a>';
			}).join("");
		});
	}

	if (document.readyState === "loading") {
		document.addEventListener("DOMContentLoaded", installSearch);
	} else {
		installSearch();
	}
})();
