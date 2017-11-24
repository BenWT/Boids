#include <Urho3D/Core/CoreEvents.h>
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
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>

#include "main.h"
#include "Character.h"
#include "Touch.h"
#include "Boids.h"
#include "Missile.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(Main)

BoidSet boids;
MissileSet missiles;

Main::Main(Context* context) : Sample(context) {}

Main::~Main() {}

void Main::Start() {
    Sample::Start();
    if (touchEnabled_)
        touch_ = new Touch(context_, TOUCH_SENSITIVITY);

    OpenConsoleWindow();
	CreateMainMenu();
}

Button* CreateButton(const String& text, int pHeight, Font* font, Urho3D::Window* window) {
    Button* button = window->CreateChild<Button>();
    button->SetMinHeight(pHeight);
    button->SetStyleAuto();

    Text* buttonText = button->CreateChild<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    window->AddChild(button);

    return button;
}

LineEdit* CreateLineEdit(const String& text, int pHeight, Urho3D::Window* window) {
    LineEdit* lineEdit = window->CreateChild<LineEdit>();
    lineEdit->SetMinHeight(pHeight);
    lineEdit->SetAlignment(HA_CENTER, VA_CENTER);
    lineEdit->SetText(text);
    lineEdit->SetStyleAuto();

    window->AddChild(lineEdit);

    return lineEdit;
}

void Main::CreateMainMenu() {
    Sample::InitMouseMode(MM_RELATIVE);
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();
    UIElement* root = ui->GetRoot();
    XMLFile* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    root->SetDefaultStyle(uiStyle);

    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto(uiStyle);
    ui->SetCursor(cursor);

    window_ = new Window(context_);
    root->AddChild(window_);

    window_->SetMinWidth(384);
    window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window_->SetAlignment(HA_CENTER, VA_CENTER);
    window_->SetName("Window");
    window_->SetStyleAuto();

    Button* connectButton = CreateButton("Connect", 24, font, window_);
    serverAddress = CreateLineEdit("localhost", 24, window_);
    Button* disconnectButton = CreateButton("Disconnect", 24, font, window_);
    Button* startButton = CreateButton("Start Server", 24, font, window_);
    Button* quitButton = CreateButton("Quit", 24, font, window_);

    SubscribeToEvent(connectButton, E_RELEASED, URHO3D_HANDLER(Main, HandleConnect));
    SubscribeToEvent(disconnectButton, E_RELEASED, URHO3D_HANDLER(Main, HandleDisconnect));
    SubscribeToEvent(startButton, E_RELEASED, URHO3D_HANDLER(Main, HandleStart));
    SubscribeToEvent(quitButton, E_RELEASED, URHO3D_HANDLER(Main, HandleQuit));
    SubscribeToEvents();
}

void Main::CreateScene(bool isClient) {
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>(LOCAL);
    scene_->CreateComponent<PhysicsWorld>(LOCAL);

    cameraNode_ = new Node(context_);
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
    camera->SetFarClip(300.0f);

    Node* zoneNode = scene_->CreateChild("Zone", LOCAL);
    Zone* zone = zoneNode->CreateComponent<Zone>(LOCAL);
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

    Node* lightNode = scene_->CreateChild("DirectionalLight", LOCAL);
    lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
    Light* light = lightNode->CreateComponent<Light>(LOCAL);
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);

    Node* floorNode = scene_->CreateChild("Floor", LOCAL);
    floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
    floorNode->SetScale(Vector3(200.0f, 1.0f, 200.0f));
    StaticModel* floorObject = floorNode->CreateComponent<StaticModel>(LOCAL);
    floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    floorObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

    RigidBody* rigidbody = floorNode->CreateComponent<RigidBody>(LOCAL);
    rigidbody->SetCollisionLayer(2);
    CollisionShape* shape = floorNode->CreateComponent<CollisionShape>(LOCAL);
    shape->SetBox(Vector3::ONE);

    GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

    if (isClient) {
        // load client stuff
    } else {
        boids.Initialise(cache, scene_);
        missiles.Initialise(cache, scene_);
    }
}

void Main::SubscribeToEvents() {
    SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(Main, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(Main, HandleClientDisconnected));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Main, HandlePostUpdate));
    SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(Main, HandlePhysicsPreStep));
}

Controls Main::FromClientToServerControls() {
    Input* input = GetSubsystem<Input>();
    Controls controls;

    controls.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
    controls.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
    controls.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
    controls.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
    controls.Set(1024, input->GetKeyDown(KEY_E));

    controls.yaw_ = yaw_;

    return controls;
}
void Main::ProcessClientControls() {
    Network* network = GetSubsystem<Network>();
    const Vector<SharedPtr<Connection> >& connections = network->GetClientConnections();

    for (unsigned i = 0; i < connections.Size(); ++i) {
        Connection* connection = connections[i];
        const Controls& controls = connection->GetControls();
        if (controls.buttons_ & CTRL_FORWARD) printf("Received from Client: Controls buttons FORWARD \n");
        if (controls.buttons_ & CTRL_BACK) printf("Received from Client: Controls buttons BACK \n");
        if (controls.buttons_ & CTRL_LEFT) printf("Received from Client: Controls buttons LEFT \n");
        if (controls.buttons_ & CTRL_RIGHT) printf("Received from Client: Controls buttons RIGHT \n");
        if (controls.buttons_ & 1024) printf("Received from client: E pressed \n");
    }
}

// void Main::HandleUpdate(StringHash eventType, VariantMap& eventData) {
	// using namespace Update;
    //
    // float timeStep = eventData[P_TIMESTEP].GetFloat();
    //
    // if (GetSubsystem<UI>()->GetFocusElement()) return;
    //
    // Input* input = GetSubsystem<Input>();
    //
    // if (cameraNode_ != NULL) {
    //     const float MOVE_SPEED = 20.0f;
    //     const float MOUSE_SENSITIVITY = 0.1f;
    //
    //     IntVector2 mouseMove = input->GetMouseMove();
    //     yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    //     pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    //     pitch_ = Clamp(pitch_, -90.0f, 90.0f);
    //
    //     cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    //
    //     if (input->GetKeyDown(KEY_W)) cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    //     if (input->GetKeyDown(KEY_S)) cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    //     if (input->GetKeyDown(KEY_A)) cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    //     if (input->GetKeyDown(KEY_D)) cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
    //
    //     if (input->GetKeyPress(KEY_SPACE) && missiles.canShoot) missiles.DoShoot(cameraNode_);
    // }
    //
    // boids.Update(timeStep);
    // missiles.Update(timeStep);
// }

void Main::HandlePostUpdate(StringHash eventType, VariantMap& eventData) {
    UI* ui = GetSubsystem<UI>();
    Input* input = GetSubsystem<Input>();
    ui->GetCursor()->SetVisible(menuVisible_);
    window_->SetVisible(menuVisible_);
}
void Main::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData) {
    using namespace Update;

    Network* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();

    float timeStep = eventData[P_TIMESTEP].GetFloat();

    if (serverConnection) {
        serverConnection->SetPosition(cameraNode_->GetPosition());
        serverConnection->SetControls(FromClientToServerControls());
    } else if (network->IsServerRunning()) {
        ProcessClientControls();

        boids.Update(timeStep);
    }
}
void Main::HandleConnect(StringHash eventType, VariantMap& eventData) {
    printf("HandleConnect");

    CreateScene(true);

    Network* network = GetSubsystem<Network>();
    String address = serverAddress->GetText().Trimmed();
    network->Connect((address.Empty()) ? "localhost" : address, SERVER_PORT, scene_);

    menuVisible_ = false;
}
void Main::HandleDisconnect(StringHash eventType, VariantMap& eventData) {
    printf("HandleDisconnect");
}
void Main::HandleClientConnected(StringHash eventType, VariantMap& eventData) {
    printf("HandleClientConnected");

    using namespace ClientConnected;

    Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    newConnection->SetScene(scene_);
}
void Main::HandleClientDisconnected(StringHash eventType, VariantMap& eventData) {
    printf("HandleClientDisconnected");

    using namespace ClientConnected;
}
void Main::HandleStart(StringHash eventType, VariantMap& eventData) {
    printf("HandleStart\n");

    Network* network = GetSubsystem<Network>();
    network->StartServer(SERVER_PORT);

    menuVisible_ = false;
    CreateScene(false);
}
void Main::HandleQuit(StringHash eventType, VariantMap& eventData) {
    engine_->Exit();
}
