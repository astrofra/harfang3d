hg = require("harfang")

function CreateCollisionNode(scene, name, collision_type, position, body_type, mass)
    local node = scene:CreateNode()
    node:SetName(name)
    node:SetTransform(scene:CreateTransform(position))

    local rigid_body = scene:CreateRigidBody()
    rigid_body:SetType(body_type)
    node:SetRigidBody(rigid_body)

    local collision = scene:CreateCollision()
    collision:SetType(collision_type)
    collision:SetMass(mass)
    if collision_type == hg.CT_Cube then
        collision:SetSize(hg.Vec3(1, 1, 1))
    else
        collision:SetRadius(0.5)
        if collision_type == hg.CT_Capsule then
            collision:SetHeight(1.0)
        end
    end
    node:SetCollision(0, collision)
    return node
end

scene = hg.Scene()

capsule_sphere = CreateCollisionNode(scene, "Capsule/Sphere capsule", hg.CT_Capsule, hg.Vec3(0, 0, 0), hg.RBT_Dynamic, 1)
sphere = CreateCollisionNode(scene, "Capsule/Sphere sphere", hg.CT_Sphere, hg.Vec3(0.8, 0, 0), hg.RBT_Static, 0)

-- Create the cube first to exercise the reversed cube/capsule dispatch order.
cube = CreateCollisionNode(scene, "Capsule/Cuboid cuboid", hg.CT_Cube, hg.Vec3(10.8, 0, 0), hg.RBT_Static, 0)
capsule_cube = CreateCollisionNode(scene, "Capsule/Cuboid capsule", hg.CT_Capsule, hg.Vec3(10, 0, 0), hg.RBT_Dynamic, 1)

capsule_a = CreateCollisionNode(scene, "Capsule/Capsule A", hg.CT_Capsule, hg.Vec3(20, 0, 0), hg.RBT_Dynamic, 1)
capsule_b = CreateCollisionNode(scene, "Capsule/Capsule B", hg.CT_Capsule, hg.Vec3(20.8, 0, 0), hg.RBT_Static, 0)

physics = hg.SceneBullet3Physics()
physics:SceneCreatePhysicsFromAssets(scene)

pairs = {
    {name = "capsule-sphere", capsule = capsule_sphere, other = sphere},
    {name = "capsule-cuboid", capsule = capsule_cube, other = cube},
    {name = "capsule-capsule", capsule = capsule_a, other = capsule_b}
}

for _, pair in ipairs(pairs) do
    assert(physics:NodeHasBody(pair.capsule), pair.name .. " capsule has no physics body")
    physics:NodeStartTrackingCollisionEvents(pair.capsule, hg.CETM_EventAndContacts)
    physics:NodeStartTrackingCollisionEvents(pair.other, hg.CETM_EventAndContacts)
end

local step = hg.time_from_ms(1)
physics:StepSimulation(step, step, 1)
contacts = physics:CollectCollisionEvents(scene)

local results = {}
for _, pair in ipairs(pairs) do
    local pair_contacts = hg.GetNodePairContacts(pair.capsule, pair.other, contacts)
    local reverse_contacts = hg.GetNodePairContacts(pair.other, pair.capsule, contacts)
    local contact_count = math.max(pair_contacts:size(), reverse_contacts:size())
    assert(contact_count > 0, pair.name .. " produced no contact")
    table.insert(results, string.format("%s=%d", pair.name, contact_count))
end

print("Capsule collision pairs QA passed: " .. table.concat(results, ", "))

scene:Clear()
scene:GarbageCollect()
