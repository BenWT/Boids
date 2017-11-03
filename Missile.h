#pragma once
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Container/Vector.h>

namespace Urho3D
{
    class Node;
    class Scene;
    class RigidBody;
    class CollisionShape;
    class ResourceCache;
}
// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

const static int NumMissiles = 100;

class Missile {
public:
    Node* pNode;
    StaticModel* pObject;
    RigidBody* pRigidBody;
    CollisionShape* pCollisionShape;
    float timer;

    // Methods
    Missile();
    void Initialise(ResourceCache *pRes, Scene *pScene);
    void DoShoot(Node *pCamera);
    void Update(float tm);
};

class MissileSet {
public:
    bool canShoot;
    float timer = 0.0f;

    // Methods
    MissileSet();
    void Initialise(ResourceCache *pRes, Scene *pScene);
    void DoShoot(Node *pCamera);
    void Update(float tm);

private:
    Vector<Missile> missileList;
};
