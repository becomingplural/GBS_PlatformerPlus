# A (Rough) Guide to the Platformer+ Settings

Updated for the release of P+ 1.

The Platformer+ engine plugin for GBStudio has been out for a few months and has gone
through some pretty drastic expansions and modifications in that time. So I thought I would
document what the different settings do, where there are small issues, and where some future
developments might take place. If you have questions about the plugin, or discover any bugs, I
regularly check the GBStudio discord and usually can get a patch together in a day or two.
Thanks for all your support, and please drop me a line when you make something with P+, I
always love to see what people create!

**New Settings**
Platforms

Jumping
Horizontal Motion
Dashing

**New Events**
Engine Field Events with Platformer Plus
Platformer+ Player Fields
Store Platformer+ Fields in Variable
Store Platformer+ State in Variable
Set Platformer + State
Enable Actor Gravity
Disable Actor Gravity


## Platformer Plus Level Settings

Platformer+ includes several options for changing how game elements operate. Some of these
have been pulled out into two other complementary plugins, PlatformerCamera and
PlatformerPlusGravity. These are designed to work with Platformer+, but can also be removed
if you don’t need them. These settings have been organized into a separate settings window to
make them easier to adjust.

```
Camera Follow Directions : Allows you to specify thedirections that the camera will
track the player. Note: by itself this doesn’t lock the player to the camera’s edge, it is
only about the camera’s view. The list of options includes settings for all the possible
combinations of edges, though you may have to look.
```
```
Camera Forward Focus : This uses the player’s velocityto move the camera slightly
further ahead in the direction of travel. The higher the number the further ahead the
camera will move. If you use forward focus, I recommend turning the camera catch-up
speed (below) to 3 or more so that the transition is smooth.
0 No forward focus
1-48 Multiplier of player velocity
```
```
Camera Catch-Up Speed : This allows the camera to followthe player with a slight
delay. The camera will move towards the player faster when they are further away and
slower as it gets close.
0 Instantly follows the player
```

```
1-6 The current distance is divided by this power of 2.
```
```
Camera Deadzone X : This is a built-in GBStudio variable,but one that normally isn’t
exposed. It has a big effect on how a platforming game feels though, so I wanted to
make it editable.
```
```
Lock Player to Camera Edge : Allows you to constrainthe player to the camera’s
current edge. This is designed to be used with either the camera move event, or with
the camera follow setting above. It only allows locking the left and/or right edge.
```
```
DropThroughPlatforms :Thisallowstheplayertodropthroughcollisiontilesthatare
settocollideonlyfromthetop.Theoptionscontrolswhatbuttoncausesthedropto
happen.
Off Disables drop-through.
Down (hold) Holding down drops through any top-collision tiles.
Down (tap) Pressing down drops through a singleplatform.
Jump and Down Pressing these two keys togetherdrops through.
```
```
PlatformActorCollisionGroup :Thisoptionallowsyoutomakeactorsintoplatforms
whilemaintainingthefullfunctionalityofactors(theycanmove,andrunon_hitevents).
Theseareactorsthatyousettoaspecificcollisiongroup,andthenspecifythatcollision
grouphere.Thepluginwillattachtheplayertothetopofanyactorinthatgroup,when
theplayerdescendsfromabove.Nootherdirectionsareaffected.Controllingtheactual
movementoftheplatforms,disablingthem,oranyotheractionsareallhandedthrough
scripting.
None (default): Disables platform actors.
Collision Group X Enables, and sets the collision group for moving platforms
```
```
Solid Actor Collision Group : This option transformsan actor into a solid object that the
player cannot pass through. The actor can move, push the player, and will run collision
scripts. The solidity applies to all directions, so the player can land on the actor or bump
their head while jumping. Currently the wall slide ability does not work with actors
however. To enable this setting, specify the collision group of the actors that you want
to be solid.
```
## Jumping

The options in jumping are interconnected with many of the default options from the
platformer engine to ensure compatibility.Unfortunately thatmeans you’llhave to move
betweenthetwopanels.Mostoftheseoptionsaresubtletweaksthatchangethe‘game-feel’


ofajump.ThePlatformer+eventpluginallowsyoutotrackwhethertheplayerisgroundedor
iscurrentlyjumping.Thejumpstatelastsforthenumberofjumpframesyouhavesetwhilethe
player is holding the jump button (but not, for instance, while falling after a jump).

```
Minimum Jump Height,
Jump Frames
Jump Velocity,
Gravity While Jumping
Tounderstandjumping inPlatformer+, we need totake a lookat howthesefour
elements all fit together.
IntheoriginalGBSplatformerengine,JumpVelocityaddsacertainamountof
upwardforcetotheplayeratthemomenttheypressthejumpbutton.Gravitywhile
jumpingappliesadifferentamountofdownwardforcethroughouttheupwardpartof
thejump,aslongastheplayerisholdingthejumpbutton.Thisgivestheplayersome
controlovertheheightofthejump(aslongasthegravityissettoalowernumberthan
regulargravity.Tovisualizethisdifferencewecangraphtheplayer’sheightovertime
like this:
```
```
Inthefirstgraph,wehaveconsistentgravity;inthesecond,thegravityislessduring
the upward part–so the player has more time to maneuver, before accelerating quickly.
Thatextracontrolisgood,butwecandobetter!SteveSwink,inhisbookGame
Feel, usesthe terminologyofmusical instrumentADSRenvelopes todescribe how
gamemechanicsreacttoinput.ADSRstandsforAttack,Decay,Sustain,andRelease.
Wedon’tneedtogettoodeepintothis,butthenewjumpmodelinPlatformer+adds
somemorecontrolstoshapethejump.First,theminimumjumpheightreplacesjump
velocityastheforceappliedonthefirstframe.Jumpvelocitynowhasadifferentrole:it
isappliedforafixednumberofframes–setwiththeJumpFramesinput–atthestartof
thejump.Sowehavetwokindsofupwardmotion:thefirstboost,followedby1-
extraframesofforce.Wealsokeepthetwokindsofgravity.Thatmeanstherearefour
```

phasestothejumpwithdifferentdynamics:thefirstframe,theboostfromthejump
frames,thetimeafterthejumpframeswhilethelowergravitykicksin,andfinallythe
descent. It looks something like this:

```
Note 1 :IfyouwanttoapproximatetheolderstyleGBStudiogravityinthisnewmodel,
setthejumpframesto1,theJumpVelocityto0,andcopyyouroldjumpvelocitytothe
Minimum Jump Height field.
Note 2 :Becausethevariablethatholdsverticalvelocitycan’tgoanyhigherthan32,
theenginehastodosometrickstomakesurethatthecombinedjumpforcesdon’t
exceedthatnumber.IfyoumaxoutJumpHeightMin,andJumpVelocity,whilereducing
the frames to 1, the engine will overwrite your settings during run-time.
```
**Extra Jumps:** This setting allows the player to jumpa second time in mid-air. That
second jump uses the same dynamics as the ground jump. You can give the player more
than one extra jump by setting the number higher. The counter resets every time the
player touches the ground.
0 Disable air-jumping
1-254 Sets the number of jumps the player can perform consecutively
255 Infinite jumps

**Height Reduction on Subsequent Jumps:** This reducesthe overall jump power of each
consecutive air-jump. This is the effect of multi-jump in Kirby games, for instance. The
numbers need to be relatively high (~4000) to be noticeable. This option only takes
effect if the Extra Jumps setting is on.

**Coyote Time** : A common problem in platforming gamesis that the player presses the
jump button just after the character has left a ledge. Because the character isn’t
touching the ground, the default GBS Platform Engine won’t allow them to jump. Coyote
time is a simple counter that keeps track of how long ago the player last touched the


ground, and if it was recently enough it allows the player to jump even though they are
in mid-air. This variable is measured in frames.
0 No coyote time.
1-10 Number of frames to allow jumping after being on a platform.

**Jump Buffer** : The flips side of coyote time. Sometimesa player will hit the jump button
too soon, right before they land. Again, the GBS engine treats this as the player still
being in the air. The jump buffer keeps track of how long ago the jump button was
pressed, and if the player lands soon afterwards then it automatically triggers a jump.
0 No jump buffer.
1-20 Number of frames to buffer the jump button for.

**Wall Jumps** : This allows the player to jump when theyare pressed up against the wall.
For this to trigger, the player must be directly against the wall and pushing into it when
they hit jump. By itself, this can be a little tricky to get the timing right. By enabling
wall-slide, it becomes much easier, and the player gets a wall-jump buffer like the
regular jump buffer. The variable is the number of consecutive wall jumps the player
can perform before touching the floor again.
0 Disable wall-jumping
1-255Sets the number of consecutive wall jumps.

**Enable Wall Slide:** This is a simple toggle that turnson or off the wall slide ability. The
speed is set below with the wall-slide gravity. Turning this on also changes the way a
player interacts with a wall, so that it change the direction the player is facing to show a
different animation, and can help with dashing in the correct direction.
Off Disables wall-sliding
On Enables wall-sliding

**Wall Slide Gravity** : Controls the speed at which thecharacter descends while attached
to a wall. The player will descend at a constant rate (whereas normal gravity is
cumulative).
0 Stick to the wall
>0 Slowly slide down the wall while pressing into it.

**Wall Kick Off:** When wall-jumping, the player oftenhas to rapidly switch from pushing
the directional pad one way to pushing it the other, and this can make wall-jumping
frustrating. Additionally, if the character remains pushed up against the wall, then the
vertical component of a jump can get cut off. The Wall Kick Off variable controls how
much outward force is applied to the avatar whenever they wall jump.


```
Note that this variable cannot be turned off entirely when wall-jumping. There is
a minimum amount of kick-off necessary.
```
```
Float Input : This option enables the Float mechanicand determines what key the
player uses for it. Floating allows the player to descend at a constant, usually slower,
rate than gravity.
None Disable the float mechanic
Hold Jump Float automatically if you hold jump after you start descending
Hold Up Float while holding the up arrow and descending.
```
```
Float Fall Speed : This variable sets the speed atwhich the float mechanic descends.
```
## Horizontal Motion

The horizontal motion controls are a bit of a varied category. These options generally change
how the player accelerates and decelerates to create slightly different feelings while walking
and running. The Platformer+ event plugin allows you to track when the player is running, and
what the intensity of that run-state is (from 0-4).

```
Air Control : When people jump in real life, they can’tchange direction mid-air.
Platforming avatars, however, often can and we call this air-control. Some game
designers like to disable this to create more realistic or cinematic effects, as in games
like Prince of Persia or Another World. By disabling air-control, the player’s horizontal
velocity is set by the speed they are traveling when they press jump, and it doesn’t
change until they land or hit a wall.
```
```
Change Avatar Direction in Air : By default, GBStudiokeeps your avatar oriented in the
direction you were facing when you started the jump. This option updates the avatar to
face in the direction of the most recent key-press.
```
```
Air Deceleration : By default, the GBS Platform Enginedoes something similar to
disabling air control because it doesn’t slow the character down in the air. So if the
player doesn’t press left or right, their character will keep the same momentum. Air
deceleration adds a bit of friction so that the character will come to a stop unless the
player actively presses a button.
```
```
Run Style : By default, the GBS run command adds asmall amount of acceleration to
the character every frame, smoothing accelerating it from a stand-still to a full run. That
is often good, but there are other ways of conceptualizing how running should feel and
work. This option allows you to select from additional styles.
```

```
No Running : Disables the run ability entirely.
Default Smooth Acceleration : GBStudio’s standard runmodel
Enhanced Smooth Acceleration : Uses walk accelerationuntil the player gets to
their full walk speed. Also uses turn acceleration (below).
Immediate Run Speed : Player instantly acceleratesto full speed.
Two Run Speed Levels : Keeps track of accelerationbehind the scenes. The
player walks at a normal speed until they’ve built up enough acceleration to
match their full run speed, at which point it switches over.
Three Run Speed Levels : As above, but adds a middletier in between walking
and running.
```
```
Acceleration when Turning : By default, GBStudio allowsyour character to turn
instantly while running at full tilt. Some of the run options above now allow you to keep
some of that momentum when the character turns. This option sets the speed with
which the player bounces back.
0 Disable turn acceleration
```
```
Jump Boost from Run Speed : This option allows youto add more height to a jump
based on the speed that the player is moving horizontally. It calculates the height based
off the speed, rather than the accumulated acceleration–so the different run speed tiers
will give consistent boosts, while the boost from smooth acceleration will vary each
frame.
Note : the code for this has been re-written in v1.4to offer more dramatic
changes in height. You may need to adjust your prior settings.
```
## Dashing

The dash is the most complex new mechanic implemented by Platformer+ and there are a
number of different options for players to tweak about how the dash functions. However there
are still many ways that you might want to tweak the dash, and you can do so by using the
Platformer+ event plugin to read the plat_state variable and check if the player is dashing. That
will allow you to tweak the animation, add effects during the dash, and tweak its final
moments.

```
Dash Input : This option enables or disables dashingin your game, and determines
what key press it is assigned to. There are three options: double-tap (left or right), the
interact button, and down and jump. Down-and-jump is primarily meant for games that
use dashing to implement a slide mechanic, and isn’t recommended if you allow for air
dashing.
```

**Dash Style** : Allows you to select if the player can dash only when touching the ground,
only while in the air, or both. Note that the ground dash only requires you to be touching
the ground when you start, and will send the character over the edges of platforms. A
future update may add an option to ground dash only to the edge of a platform.

**Dash Momentum:** Dashing, unlike regular movement, triesto travel a fixed distance
from its start point to its end point. This option allows you to control what other forces
come into play on that trajectory. If you enable horizontal momentum, the character will
end a dash moving at their full running speed. If you enable vertical momentum, the
character will be able to jump while dashing (think Mega Man X) and gravity will pull
them down. If you turn off dash momentum, the character will travel a straight line and
stop at the end of it.
**Note** : The option to wall-dash below requires thatthe dash calculate whether or
not there is an open space at the end of its trajectory. Adding gravity and vertical
collisions makes it impossible to predict that end-point (at least in GBStudio). So if you
enable vertical momentum, you cannot dash through walls.
**Note 2** : The code for dash-jumping has been re-written.The results are much
more pleasing!

**Dash Through** : This setting allows you to change howthe dash interacts with objects
along its path. It doesn’t change anything about the interactions at the landing site–so if
you dash into an enemy and land on them, you will take damage as normal. However,
you can disable interactions on the path between the start and the finish. There are
three levels of interactions you can disable. You can turn off actor interactions, which
means that the player won’t trigger any scripts on actors in their path. You can turn off
actor interactions AND trigger interactions, so that the player will not run any scripts in
trigger areas. Finally, you can turn off wall collisions as well. If you turn off collisions,
the dash still calculates whether its ultimate end position is within a wall or not, and if it
would put the player in a wall it chooses instead to dash normally. Sometimes this
means the player thinks they should be able to dash through something but it doesn’t
work. Future versions may have a fix for this, but currently it is working as designed.
**Note** : Actor interactions are expensive to calculate,so P+ only makes use of the one
check that is typically run in the basic engine. That means that if you set Dash Time to a
low enough number, and the distance high enough, you will dash through actors even if
Dash-Through is set to None.

**Dash Distance** : The total distance covered by the dash.Divide this by Dash Time to get
the distance traveled each frame. Setting this really high and Dash Time really low


```
means that the camera has to quickly catch up to the player. There is some basic
camera smoothing to help with that jitter, but it can still be a little odd.
```
```
Dash Time (frames) : The time it takes for the dashto cross its total distance. This is
measured in frames, and Game Boy games run at 60 frames per second. I’ve capped
this number at 30, but if you feel like you want to increase the cap for any reason, it’s
easy to edit in the engine.json.
```
```
Dash Recharge Time : The time (in frames) before theplayer can dash again after
previously using a dash. Currently the dash doesn’t have any recharge setting for
touching the ground–so you can air-dash multiple times in a row if the recharge time is
low enough. In the future there will probably be an option for resetting the dash when
landing.
```
## Engine Field Events with Platformer Plus

One of the cool things about combining Platformer+ with GBS 3.1 is that we now have events
that can change engine fields. That means that you can have power-ups that enable
double-jumping, wall-jumping, dashing, or floating. It also means that you can change the
gravity or other aspects of the physics on the fly to create slippery zones or shorter jumps.
However, there are a couple of caveats about making changes to some of the jumping
and dashing numbers in Platformer+. The amount of jump force applied during each jump
frame is calculated when the scene starts, as is the amount of dash distance applied during
each frame of dashing. That means that any changes you make to the Jump Velocity, Jump
Frames, Dash Distance, or Dash Time, won’t register until you move into the next scene. The
initialization phase also has some safety checks to make sure the variables all stay within a
valid range–however there are some creative ways to bypass those checks when changing
things during gameplay. If your character’s jump ever stops working when you update an
engine variable, try dialing back the amount that you’re increasing that variable.

## Platformer+ Player Fields

In addition to the engine fields, Platformer+ comes with some additional custom events for
monitoring its unique engine fields.

**Store Platformer+ Fields in Variable:**
This event allows you to check the value of some useful variables.
**Player on Moving Platform** : True if the player isatop to a platform actor or solid actor.
**Current Run Stage** : Useful for tracking how much accelerationthe player has
accumulated from running.


```
-1 = The player is moving in the opposite direction from the keypress.
0 = The player is not running
1 = Top speed for immediate and default smooth running.
2 = Top speed for enhanced smooth running.
3 = Top speed for Two-tier running
4 = Top speed for Three-tier running.
```
```
Current Jump Type : Tracks the type of jump the playeris performing.
0 = Not jumping
1 = Jumping from the ground
2 = Jumping in the air (ie. double-jump)
3 = Jumping from a wall
4 = Floating
Frames of Coyote Time Left : let’s you know if a playercan jump even if they aren’t
grounded.
Dashing is Frozen : Not currently implemented.
```
**Store Platformer+ State in Variable:**
Platformer+ uses a state machine to track the player’s behavior. This means that the player can
only ever be in a single ‘state’ at a time. You can find out which state using this event. The
values are as follows:
0 = Entering the falling state (1 frame)
1 = Falling
2 = Entering the grounded state
3 = On the ground
4 = Entering the jumping state
5 = Jumping
6 = Entering the dashing state
7 = Dashing
8 = Entering the climbing state
9 = On a Ladder
10 = Entering the wall-slide state
11 = On a Wall
12 = Entering the knock-back state
13 = Knockback state
14 = Entering the blank state
15 = Blank state


**Set Platformer + State:**
You can now directly set the player’s state in Platformer+. Some of the most useful cases
include:
Set to Fall State: You can use this to interrupt a dash or a jump.
**Set to Jump State** : You can use this to create a jumpthrough code, attach it to a
button, or add it to the end of another state like dashing.
**Set to Dash State** : If you want the player to dashin circumstances that aren’t covered
by Platformer+, you can use this state to create new dashes. For instance, dashing
backwards.
**Set to Knockback State** : In the knockback state, theplayer will still feel the effects of
gravity and other forces, and will still collide with walls in physical ways. However the
player cannot input any commands and the animation state will not change by itself.
**Set to Blank State** : The blank state is even more dramatic.It zeros out any prior
velocity that the player had, and removes collisions with walls (though it keeps
collisions with triggers and enemies). Its useful for cinematic events where you want
the rest of the game to keep moving while the player is being directly controlled.

**Enable Actor Gravity:**
Platformer Plus now allows you to give any actor to check whether they are grounded, to fall at
the speed of gravity when they aren’t, and to collide properly with floors. The ground checks
happen once every 8 frames.

**Disable Actor Gravity:**
Gravity on actors can hog some system resources. When you’re no longer using it, turn it back
off again.

