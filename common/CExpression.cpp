//
// Cexpression.cpp
// Copyright Menace Software (www.menasoft.com).
//

#include "../graysvr/graysvr.h"

//	Suggested including default values here, allowing to override in DEFMESSAGE blocks
//		so, they could come out commented in revisions, enabled only if needed
TCHAR CExpression::sm_szMessages[DEFMSG_QTY+1][256] =
{
	"You have no clue how to make this potion.",
	"Hmmm, you lack %s for this potion.",
	"You have no bottles for your potion.",
	"That is not a reagent.",
	"pour the completed potion into a bottle",
	"start grinding some %s in the mortar",
	"add %s and continue grinding",
	"toss the failed mixture from the mortar",
	"very clumsy",
	"superhumanly agile",
	"somewhat uncoordinated",
	"moderately dexterous",
	"somewhat agile",
	"very agile",
	"extremely agile",
	"extraordinarily agile",
	"like they move like quicksilver",
	"like one of the fastest people you have ever seen",
	"This is a magical creature",
	"%s looks %s and %s.",
	"rather feeble",
	"superhumanly strong",
	"somewhat weak",
	"to be of normal strength",
	"somewhat strong",
	"very strong",
	"extremely strong",
	"extraordinarily strong",
	"as strong as an ox",
	"like one of the strongest people you have ever seen",
	"like a conjured creature",
	"%s looks %s.",
	"%s is %s own master",
	"%s is loyal to %s",
	"you",
	"%s is a %s",
	"Attack [%i].",
	"That doesn't look dangerous at all.",
	"That weapon is extremely deadly.",
	"That might hurt someone a little bit.",
	"That does small amount of damage.",
	"That does fair amount of damage.",
	"That does considerable damage.",
	"That is a dangerous weapon.",
	"That weapon does a large amount of damage.",
	"That weapon does a huge amount of damage.",
	"That weapon is deadly.",
	"Defense [%i].",
	"That offers no protection.",
	"That makes you almost invincible.",
	"That might protect a little bit.",
	"That offers a little protection.",
	"That offers fair protection.",
	"That offers considerable protection.",
	"That is good protection.",
	"That is very good protection.",
	"That is excellent protection.",
	"That is nearly the best protection.",
	"You see no poison.",
	"You see enough poison to kill almost anything.",
	"You detect a tiny amount of poison.",
	"You find a small amount of poison.",
	"You find a little poison.",
	"You find some poison.",
	"There is poison on that.",
	"You find a lot of poison.",
	"You find a large amount of poison.",
	"You find a huge amount of poison.",
	"This item is %s.",
	" It looks quite fragile.",
	"That does not appear to be a weapon or armor.",
	"You grovel at %s's feet",
	"Your bankbox can't hold more items.",
	"Your bankbox can't hold more weight.",
	"%s has %d stones in %s %s",
	"You have %d stones in your %s",
	"You can't make that.",
	"You can't make anything with what you have.",
	"You can't use the %s where it is.",
	"You have no blank parchment to draw on",
	"You have no blank parchment to draw on",
	"You can't seem to figure out your surroundings.",
	"Guards can now be called on you.",
	"You carve the corpse but find nothing usefull.",
	"You pluck the bird and get some feathers.",
	"You skin the corpse and get some fur.",
	"You skin the corpse and get the hides.",
	"You carve away some meat.",
	"You skin the corpse and get some unspun wool.",
	"Not a valid command or format",
	"You go into AFK mode",
	"You leave AFK mode",
	"You have no ammunition.",
	"You are too close.",
	"*You see %s attacking %s*",
	"*%s is attacking you*",
	"You hit %ss left arm!",
	"%s hits your left arm!",
	"You hit %ss right arm!",
	"%s hits your right arm!",
	"You hit %ss right arm!",
	"%s hits your right arm!",
	"You score a hit to %ss back!",
	"%s scores a hit to your back!",
	"You hit %ss Chest!",
	"%s hits your Chest!",
	"You land a blow to %ss stomach!",
	"%s lands a blow to your stomach!",
	"You hit %s in the ribs!",
	"%s hits you in the ribs!",
	"You land a terrible blow to %ss chest!",
	"%s lands a terrible blow to your chest!",
	"You knock the wind out of %s!",
	"%s knocks the wind out of you!",
	"You smash %s in the rib cage!",
	"%s smashed you in the rib cage!",
	"You hit %s foot!",
	"%s hits your foot!",
	"You hit %ss left hand!",
	"%s hits your left hand!",
	"You hit %ss right hand!",
	"%s hits your right hand!",
	"You hit %ss right hand!",
	"%s hits your right hand!",
	"You hit %s straight in the face!",
	"%s hit you straight in the face!",
	"You hit %s on the head!",
	"%s hits you on the head!",
	"You hit %s square in the jaw!",
	"%s hit you square in the jaw!",
	"You score a stunning blow to %ss head!",
	"%s scores a stunning blow to your head!",
	"You smash a blow across %ss face!",
	"%s smashes a blow across your face!",
	"You score a terrible hit to %ss temple!",
	"%s scores a terrible hit to your temple!",
	"You hit %ss left thigh!",
	"%s hits your left thigh!",
	"You hit %ss right thigh!",
	"%s hits your right thigh!",
	"You hit %s in the groin!",
	"%s hits you in the groin!",
	"You hit %s in the throat!",
	"%s hits you in the throat!",
	"You smash %s in the throat!",
	"%s smashes you in the throat!",
	"%s missed you.",
	"You miss %s.",
	"You parry the blow",
	"Too many items in that container",
	"The item bounces out of the magic container",
	"The container is too small for that",
	"Mmm, smells good",
	"%s of %s",
	"You find %s",
	"You can't move the item.",
	"You can't drink another potion yet",
	"dumb as a rock",
	"superhumanly intelligent",
	"fairly stupid",
	"not the brightest",
	"about average",
	"moderately intelligent",
	"very intelligent",
	"like a formidable intellect",
	"extraordinarily intelligent",
	"like a definite genius",
	"clueless about magic",
	"to have vague grasp of magic",
	"capable of using magic",
	"higly capable with magic",
	"to be adept of magic",
	"to have mastery of magic",
	"nearly absent of mana",
	"low on mana",
	"half drained of mana",
	"have a high charge of mana",
	"full of mana",
	"super charged with mana",
	"%s looks %s.",
	"They look %s and %s.",
	"Try fishing elsewhere",
	"There are no fish here.",
	"You can't fish from where you are standing.",
	"Where would you like to fish?",
	"That is too far away.",
	"You pull out a %s!",
	"You are not capable of eating.",
	"You are simply too full to eat any more!",
	"You can't move the item.",
	"You eat the food, but are still extremely hungry.",
	"After eating the food, you feel much less hungry.",
	"You eat the food, and begin to feel more satiated.",
	"You are nearly stuffed, but manage to eat the food.",
	"You feel quite full after consuming the food.",
	"You are stuffed!",
	"You can't really eat this.",
	"%s is unconscious but alive.",
	"This is the corpse of %s and it is has been carved up. ",
	"It looks to have been carved by %s",
	"Forensics must be used on a corpse.",
	"You cannot determine who killed it",
	"It looks to have been killed by %s",
	"You are too far away to tell much.",
	"This is %s and it is %d seconds old. ",
	"Describe your comment or problem",
	"%s crafted by %s",
	"Your resurrection attempt is blocked by antimagic.",
	"%s is attempting to apply bandages to %s",
	"%s is attempting to apply bandages to %s, but has failed",
	"This creature is beyond help.",
	"Put the corpse on the ground ",
	"You cure %s of poisons!",
	"%s has cured you of poisons!",
	"You can't heal a ghost! Try healing their corpse.",
	"You are healthy",
	"Your healing was interrupted by the hit!",
	"Where are your bandages?",
	"Try healing a creature.",
	"%s does not require you to heal or cure them!",
	"You must be able to reach the target",
	"Resurrect %s",
	"apply bandages to self",
	"apply bandages to %s",
	"You are too far away to apply bandages on %s",
	"you",
	"Use a bandage",
	"They look somewhat annoyed",
	"You lost your target!",
	"You lost your crook!",
	"The animal goes where it is instructed",
	"You have been revealed",
	"You stumble apon %s hidden.",
	"You have hidden yourself well",
	"You are too well lit to hide",
	"You have no blank scrolls",
	" It is magical.",
	" It is has a slight magical aura.",
	" It is not repairable.",
	"You estimate %d gold for '%s'",
	"It is made of ",
	"The item does not apear to have any real resale value.",
	"That appears to be '%s'.",
	"The advance gate fails!",
	"The advance gate is not configured.",
	"The target is currently blocked.",
	"poor shot destroys the %s.",
	"The target is empty.",
	"hit the outer ring.",
	"hit the middle ring.",
	"hit the inner ring.",
	"hit the bullseye!.",
	"shot is well off target.",
	"shot is wide of the mark.",
	"shot misses terribly.",
	"shot nearly misses the archery butte.",
	"You have no ammunition",
	"You need to be correctly aligned with the butte in order to use it.",
	"remove the %s%s from the target",
	"s",
	"You can't learn any more by using an archery butte.",
	"shot splits another %s.",
	"You can only train archery on this.",
	"You need to remove what is on the target before you can train here.",
	"Clean bloody bandages in water",
	"What do you want to use this on?",
	"You can't reach this.",
	"Corrupt bboard message",
	"This board message is not yours",
	"You can't reach the bboard",
	"Put this on the ground to open it.",
	"The hive appears to be unproductive",
	"Ouch!  Bee sting!",
	"You start a new bolt of cloth.",
	"The bolt of cloth needs a good deal more.",
	"The bolt of cloth needs a little more.",
	"The bolt of cloth is nearly finished.",
	"The bolt of cloth is finished.",
	"The book apears to be ruined!",
	"Your shield prevents you from using your bow correctly.",
	"Feed shot and powder into the cannon muzzle.",
	"The cannon already has powder.",
	"The cannon already has shot.",
	"Powder loaded.",
	"Shot loaded.",
	"The cannon needs powder",
	"The cannon needs shot",
	"Armed and ready. What is the target?",
	"You can't think of a way to use that item.",
	"What do you want to use the %s on?",
	"You create some thread.",
	"What would you like to herd?",
	"Try just picking up inanimate objects",
	"The dye just drips off this.",
	"You have no hair to dye!",
	"You must have the item in your backpack or on your person.",
	"Select the object to use this on.",
	"You can only use this item on a dye vat.",
	"Which dye vat will you use this on?",
	"Use a knife to cut this up",
	"You can't move this.",
	"What do you want to cook this on?",
	"You can't reach this.",
	"You must cook food on some sort of fire",
	"Select ore to smelt. ",
	"Can't open game board in a container",
	"What is the new name?",
	"You can only open the hatch on board the ship",
	"You can't reach this.",
	"The key must be on your person",
	"You don't have a key for this.",
	"That does not have a lock.",
	"Select item to use the key on.",
	"You can't light the kindling in a container",
	"You fail to ignite the fire.",
	"This item is locked.",
	"You can't move this.",
	"Use a dagger on wood to create bows or shafts.",
	"Use thread or yarn on looms",
	"You remove the previously uncompleted weave.",
	"Where do you want to use the %s?",
	"You must possess the map to get a good look at it.",
	"What reagent you like to make a potion out of?",
	"The structure is blocked.",
	"Terrain is too bumpy to place structure here.",
	"The structure collapses.",
	"No building is allowed here.",
	"%s is in your way.",
	"The ship would not be completely in the water.",
	"You have no mortar and pestle.",
	"You feel confident in your deception.",
	"You must be standing in front of or behind the dip to use it.",
	"Try practicing on real people.",
	"Fill pitcher with some liquid.",
	"You can't reach this.",
	"Where do you want to use the %s?",
	"You can't move the gate.",
	"You can't move the item.",
	"What is the new name of the rune ?",
	"Use scissors on hair or cloth to cut",
	"What do you want to use the %s on?",
	"I am in %s, %s",
	"I cannot tell where I'm at.",
	"That is locked",
	"You have a feeling of holiness",
	"You can't use a sewing kit on that.",
	"You negate the spawn.",
	"You negate the spawn and activate it.",
	"You trigger the spawn.",
	"Use wool or cotton on spinning wheels",
	"Felucca is in the %s phase.",
	"new moon",
	"waxing crescent",
	"first quarter",
	"waxing gibbous",
	"full moon",
	"waning gibbous",
	"third quarter",
	"waning crescent",
	"Trammel is in the %s phase.",
	"You must steal this first.",
	"You are stuck on the %s",
	"You cannot train archery on this.",
	"You must be standing in front of or behind the dummy to use it.",
	"You can learn alot from a dummy, but you've already learned it all.",
	"Wow you can see the sky!",
	"Arrg stop that.",
	"Target is too far away",
	"Who do you want to attune to ?",
	"You trash the item. It will decay in 15 seconds.",
	"You can't think of a way to use that item.",
	"It appears immune to your blow",
	"What do you want to use this on?",
	"You must wait for the wool to grow back",
	"You create some yarn.",
	"You need a lock pick.",
	"Your pick must be on your person.",
	"You can't reach that.",
	"Use the lock pick on a lockable item.",
	"There is 1 other player here.",
	"There are %d other players here.",
	"Try chopping elsewhere.",
	"There is nothing here to chop.",
	"This is not a tree.",
	"There is no wood left to harvest.",
	"You hack at the tree and produce some kindling.",
	"Try chopping a tree.",
	"You have no line of sight to that location",
	"That is too far away.",
	"You cannot target an item with this spell.",
	"You cannot target a character with this spell.",
	"You cannot target yourself with this spell.",
	"You must target the ground with this spell.",
	"Due to your poor skill, the item is of shoddy quality",
	"You were barely able to make this item.  It is of poor quality",
	"You make the item, but it is of below average quality",
	"The item is of above average quality",
	"The item is of excellent quality",
	"Due to your exceptional skill, the item is of superior quality",
	"You are at peace.", // already have full mana
	"You are at peace.", // reached full mana
	"You attempt a meditative trance.",
	"Try mining elsewhere.",
	"There is nothing here to mine for.",
	"There is no ore here to mine.",
	"Try mining in rock!",
	"You can't use the %s where it is.",
	"It is consumed in the fire.",
	"The fire is not hot enough to melt this.",
	"You must be near a forge to smelt",
	"Use a smith hammer to make items from ingots.",
	"You have no line of sight to that location",
	"You need to target the ore you want to smelt",
	"You smelt the %s but are left with nothing useful.",
	"That is too far away.",
	"You lack the skill to smelt %s",
	"You must use a shovel or pick.",
	"Account already in use.",
	"Bad password for this account.",
	"Account '%s' blocked",
	"Account blocked. Send email to admin %s.",
	"Sorry, but your access has been denied.",
	"Email address '%s' is not allowed.",
	"Email for '%s' is '%s'.",
	"All guest accounts are currently used.",
	"A'%s' was %sed by '%s'",
	"Must supply a password.",
	"You are not privileged to do that",
	"Unknown account name '%s'. Try using a 'guest' account.",
	"Bad login format. Check Client Version '%s' ?",
	"Already on line",
	"%s has %s %s.",
	"arrived in",
	"departed from",
	"in your pack",
	"You are not strong enough to push %s out of the way.",
	"Can't sleep here",
	"You lack privilege to do this.",
	"Body of %s",
	"You have retreated from the battle with %s",
	"%s has retreated from the battle.",
	"Criminal!",
	"eat some %s",
	"Email '%s' for account '%s' has failed %d times.",
	"Email address for account '%s' has not yet been set.",
	"Use the command /EMAIL name@yoursp.com to set it.",
	"*You see %ss %s %s*",
	"*Your %s %s*",
	"*You see %ss %s*",
	"*Your %s*",
	"*You see %s %s*",
	"*You %s*",
	"*You see %s %s*",
	"Invalid Set",
	"You can't put blank keys on a keyring.",
	"That's not a saleable item.",
	"That is not a game piece",
	"This is not a key.",
	"You are too fatigued to move.",
	"at your feet. It is too heavy.",
	"This figurine is not yours",
	"Follow the Arrow",
	"starving",
	"very hungry",
	"hungry",
	"fairly content",
	"content",
	"fed",
	"well fed",
	"stuffed",
	"You have been forgiven",
	"You are frozen and can not move.",
	"Game Master Page Cancelled.",
	"Available Game Masters have been notified of your request.",
	"There are %d messages queued ahead of you",
	"There is no Game Master available to take your call. Your message has been queued.",
	"GM Page from %s [0%lx] @ %d,%d,%d,%d : %s",
	"You have an existing page. It has been updated.",
	"There are %d GM pages pending. Use /PAGE command.",
	"Guards can now be called on you.",
	"Your guest curse prevents you from taking this action",
	"You cannot quit your %s while in a fight",
	"That is too heavy. You can't move that.",
	"You are %s",
	"You push past something invisible",
	"It is dead.",
	"You put the %s %s.",
	"You have been jailed",
	"Both keys are blank.",
	"You can't reach it.",
	"This door is locked.",
	"You fail to copy the key.",
	"The key is a blank.",
	"That does not have a lock.",
	"To copy keys get a blank key",
	"What would you like to name the key?",
	"Use a key on a locked item or another key to copy.",
	"You lock the container.",
	"You unlock the container.",
	"You lock the door.",
	"You unlock the door.",
	"You lock the hold.",
	"You unlock the hold.",
	"That does not have a lock.",
	"You can't reach it.",
	"You lock the ship.",
	"You unlock the ship.",
	"What should the sign say?",
	"The key does not fit into that lock.",
	"%c'%s' was killed by ",
	"The attack is magically blocked",
	"Drop mail on other players",
	"'%s' has dropped mail on you.",
	"You already have %d character(s).",
	"The ceiling is too low for you to be mounted!",
	"You can't reach the creature.",
	"You dont own that horse.",
	"You are not physically capable of riding a horse.",
	"Murderer!",
	"You have no spellbook",
	"You have %s %s %s.",
	"a bit of",
	"a small amount of",
	"a little",
	"some",
	"a moderate amount of",
	"alot of",
	"large amounts of",
	"huge amounts of",
	"gained ",
	"lost",
	"You are now %s",
	"You are too overloaded to move.",
	"confused",
	"very unhappy",
	"unhappy",
	"fairly content",
	"content",
	"happy",
	"very happy",
	"extremely happy",
	"confused",
	"ravenously hungry",
	"very hungry",
	"hungry",
	"a little hungry",
	"satisfied",
	"very satisfied",
	"full",
	"You shove %s out of the way.",
	"You have entered %s",
	"the",
	"You are now under the protection of %s guards",
	"You have left the protection of %s guards",
	"You are under the protection of %s guards",
	"the",
	"You lose your safety from other players.",
	"You are safe from other players here.",
	"You have a feeling of complete safety",
	"You lose your feeling of safety",
	"You are stranded in the water !",
	"You have drifted to the nearest shore.",
	"%s Renaming Canceled",
	"%s renamed: %s",
	"%s name %s is not allowed.",
	"Already a tree here",
	"These seeds are no good",
	"You can't reach this",
	"Try planting in soil",
	"Server is in admin only mode.",
	"Sorry the server is full. Try again later.",
	"Server is in local debug mode.",
	"What would you like to name the ship?",
	"That is not yours. You will have to steal the item",
	"You step on the body of %s.",
	"It will regenerate in %d seconds",
	"First object must be a dynamic item, try again.",
	"That's the same object. Link cancelled.",
	"What do you want to link it to ?",
	"These items are now linked.",
	"Pick other corner:",
	"You feel a tingling sensation ",
	"Too far away from the Vendor",
	"You are too far away to trade items",
	"You should wait %i seconds more before accepting.",
	"You are unconscious and can't move.",
	"Sorry wrestling moves not available yet",
	"You notice %s %s %s.",
	"You notice %s %s %s%s %s",
	"'s",
	"your",
	"The item has been locked down.",
	"The item has been un-locked from the structure.",
	"You have no musical instrument available",
	"You play poorly.",
	"Ahh, Guards my goods are gone !!",
	"Here you are, %d gold coin%s. I thank thee for thy business.",
	"That does not appear to be a living being.",
	"I shall deposit %d gold in your account",
	"Could thou spare a few coins?",
	"I have a family to feed, think of the children.",
	"Can you spare any gold?",
	"I have children to feed.",
	"I haven't eaten in days.",
	"Please? Just a few coins.",
	"Thank thee! Now I can feed my children!",
	"Mmm thank you. I was very hungry.",
	"If only others were as kind as you.",
	"I'll sell this for gold! Thank thee!",
	"Help! Guards a Criminal!",
	"They don't appear to want the item",
	"Well it was nice speaking to you %s but i must go about my business",
	"Nice speaking to you %s",
	"Excuse me %s, but %s wants to speak to me",
	"Help! Guards a Criminal!",
	"Help! Guards a Monster!",
	"Get thee hence, busybody!",
	"What do you think your doing ?",
	"Ack, away with you!",
	"Beware or I'll call the guards!",
	"Gold is always welcome. thank thee.",
	"Thou shalt regret thine actions, swine!",
	"Death to all evil!",
	"Take this vile criminal!",
	"Evildoers shay pay!",
	"Take this swine!",
	"%s thou art a cowardly swine.",
	"You shall get whats coming to you %s",
	"Evildoers shay pay %s!",
	"Beware of me foul %s",
	"Get thee gone foul %s",
	"I'm sorry but i cannot resurrect you",
	"Ah! My magic is failing!",
	"I can't see you clearly ghost. Manifest for me to heal thee.",
	"You are too far away ghost. Come closer.",
	"Thou art a criminal, I shall not resurrect you.",
	"Because of your lawless ways, I shall not help you.",
	"It wouldn't be right to help someone like you.",
	"Thou hast committed too many unholy acts to be resurrected",
	"Thou hast strayed from the virtues and I shall not resurrect you.",
	"Thy soul is too dark for me to resurrect.",
	"You are good, why would I help you.",
	"Change thy virtuous ways, and I may consider resurrecting you.",
	"Muhaaahaa, You have got to be kidding, I shall not resurrect such as thee.",
	"Thou art dead, but 'tis within my power to resurrect thee.  Live!",
	"Allow me to resurrect thee ghost.  Thy time of true death has not yet come.",
	"Perhaps thou shouldst be more careful.  Here, I shall resurrect thee.",
	"Live again, ghost!  Thy time in this world is not yet done.",
	"I shall attempt to resurrect thee.",
	"I can't sell this",
	"I'm carrying nothing.",
	"I have been paid to work for %d more days.",
	"%s decides they are better off as their own master.",
	"You sense that %s has deserted you.",
	"Tell me to drop this when you want it back.",
	"Sorry I am already employed.",
	"Sorry",
	"No thank you.",
	"Mmm. Thank you.",
	"Here is %d gold. I will keep 1 days wage on hand. To get any items say 'Inventory'",
	"I only have %d gold. That is less that a days wage. Tell me 'release' if you want me to leave.",
	"I can be hired for %d gold per day.",
	"I require %d gold per day for hire.",
	"I will work for you for %d days",
	"I'm sorry but my hire time is up.",
	"You can only price things in my inventory.",
	"This contains the items I have bought.",
	"Put sample items like you want me to purchase in here",
	"Put items you want me to sell in here.",
	"I will keep this money in account.",
	"Sorry thats not enough for a days wage.",
	"Sorry I am not available for hire.",
	"I will not work for you.",
	"I will attempt to sell this item for you.",
	"What item would you like to set the price of?",
	"Yes Master",
	"Who do you want to attack?",
	"What should they fetch?",
	"Who should they follow?",
	"Who is their friend?",
	"Where should they go?",
	"What should they guard?",
	"Who do you want to transfer to?",
	"Thank thee",
	"I will work for %d gold",
	"I'm too weak to carry that. ",
	"It doesn't seem to want to go.",
	"I'm sorry the stables are full",
	"I can't seem to see your pet",
	"Sorry I have no stabled animals for you",
	"Please remember my name is %s. Tell me to 'Retrieve' when you want your pet back.",
	"Select a pet to stable",
	"What pet would you like to stable here",
	"I'm sorry you have too many pets stabled here already. Tell me to 'retrieve' if you want them.",
	"Treat them well",
	"Thy conscience is clear.",
	"Although thou hast committed murder in the past, the guards will still protect you.",
	"Your soul is black with guilt. Because thou hast murdered, the guards shall smite thee.",
	"Thou hast done great naughty! I wouldst not show my face in town if I were thee.",
	"I know nothing about that",
	"I know nothing about %s",
	"You know more about %s than I do",
	"You already know as much as I can teach of %s",
	"I would never train the likes of you. ",
	"I can't remember what I was supposed to teach you ",
	"For %d gold I will train you in all I know of %s. For less gold I will teach you less",
	"For a fee I can teach thee of ",
	"more",
	", ",
	"Let me show you something of how this is done",
	"That is all I can teach.",
	"I wish i could teach more but I cannot.",
	"There is nothing that I can teach you.",
	" and ",
	"That will be %d gold coin%s. ",
	"s", // coins or coin 
	"I cannot afford any more at the moment",
	"Here you are, %s",
	"Sorry I have no goods to sell",
	"Alas I have run out of money.",
	"You have nothing I'm interested in",
	"Sorry I have nothing of interest to you.",
	"Sorry, I'm currently off-duty.  Come back when my shop is open.",
	"That is %d gold coin%s worth of goods.",
	"To trade with me, please say 'vendor sell'",
	"Setting price of %s to %d",
	"What do you want the price to be?",
	"I have %d gold on hand. ",
	"for which I will work for %d more days. ",
	"I have %d items to sell.",
	"I restock to %d gold in %d minutes or every %d minutes. ",
	"I thank thee for thy business.",
	"You have been added to the party",
	"Your party has disbanded.",
	"You have invited %s to join your party.",
	"You have been invited to join %s's party. Type /accept to join or /decline to decline offer.",
	"%s has joined the party",
	"%s has %s your party",
	"You have %s the party",
	"You have chosen to allow your party to loot your corpse",
	"You have chosen to prevent your party from looting your corpse",
	"been removed from",
	"left",
	"Select a person to join your party.",
	"Who would you like to add to your party?",
	"What poison do you want to use?",
	"You apply the poison.",
	"You can only poison food or piercing weapons.",
	"looks peaceful",
	"looks furious",
	"You can not provoke players!",
	"You are really upset about this",
	"Who do you want to provoke them against?",
	"You can only provoke living creatures.",
	"You can't reach that.",
	"Your ghostly hand passes through the object.",
	"You can't use this where it is.",
	"You can't reach it.",
	"You should use this skill to disable traps",
	"repair",
	"really damage",
	"slightly damage",
	"fail to repair",
	"destroy",
	"You must be near an anvil to repair",
	"The item is already in full repair.",
	"You lack %s to repair this.",
	"components",
	"%s the %s",
	"The item is not repairable",
	"Can't repair this where it is !",
	"You have trouble figuring out the item.",
	"Can't repair an item being worn !",
	"Resync FAILED!",
	"Server is being PAUSED for Resync",
	"Resync complete!",
	"World save has been initiated.",
	"Skill not implemented!",
	"You can't do much in your current state.",
	"You are preoccupied with thoughts of battle.",
	"You must wait to perform another action.",
	"You are violently awakened",
	"That is not a corpse!",
	"You need ingots for smithing.",
	"You must be near a forge to smith ingots",
	"You must weild a smith hammer of some sort.",
	"The ingots must be on your person",
	"Your mark is too far away.",
	"You can't reach it.",
	"peeking into",
	"attempting to peek into",
	"*Hic*",
	"The corpse stirs for a moment then falls!",
	"That is not a corpse!",
	"Cursed Magic!",
	"That is not a field!",
	"Magic items must be on your person to activate.",
	"This item lacks any enchantment.",
	"Antimagic blocks the gate",
	"The spell fizzles",
	"looks %s",
	"sickly",
	"very ill",
	"extremely sick",
	"deathly sick",
	"The recall rune is blank.",
	"The recall rune fades completely.",
	"The recall rune's magic has faded",
	"That item is not a recall rune.",
	"The recall rune is starting to fade.",
	"Anti-Magic blocks the Resurrection!",
	"You spirit rejoins your body ",
	"Resurrection Robe",
	"Can't cast a spell on this where it is",
	"Can't target fields directly on a character.",
	"Target is not in line of sight",
	"This spell has no effect on objects",
	"This spell needs a target object",
	"An anti-magic field blocks you",
	"You can't teleport to that spot.",
	"You feel the world is punishing you.",
	"You feel you should be ashamed of your actions.",
	"An anti-magic field disturbs the spells.",
	"This is beyond your ability.",
	"You don't have a spellbook handy.",
	"You lack sufficient mana for this spell",
	"You lack %s for this spell",
	"The spell is not in your spellbook.",
	"reagents",
	"It seems to be out of charges",
	"You feel %s",
	"You establish a connection to the netherworld.",
	"You can't steal from corpses",
	"They have nothing to steal",
	"You can't steal from game boards",
	"That is too heavy.",
	"Your mark is too far away.",
	"No need to steal this",
	"Nothing to steal here.",
	"Just dclick this to practice stealing",
	"You can't reach it.",
	"Can't harm other players here.",
	"stealing",
	"No stealing is possible here.",
	"You can't steal from trade windows",
	"attempting to steal",
	"I won't hurt you.",
	"I always wanted a %s like you",
	"Good %s",
	"Here %s",
	"You can't tame them.",
	"You can not see the creature.",
	"You are too far away.",
	"The %s remembers you and accepts you once more as it's master.",
	"It seems to accept you as master",
	"%s is already tame.",
	"%s cannot be tamed.",
	"You are your own master.",
	"What do you want to use this on?",
	"Try tasting some item.",
	"It tastes like %s.",
	"Yummy!",
	"You can't use this.",
	"%s to the %s",
	" near",
	" ",
	" far",
	" very far",
	"Tracking Cancelled",
	"You see no signs of animals to track.",
	"You see no signs of monsters to track.",
	"You see no signs of humans to track.",
	"You see no signs of players to track.",
	"You see no signs to track.",
	"You need a Spyglass to use tracking here.",
	"You cannot locate your target",
	// additional messages will be added here in unsorted fashion
	NULL,
};

LPCTSTR const CExpression::sm_szMsgNames[DEFMSG_QTY+1][32] =
{
	"alchemy_dunno",
	"alchemy_lack",
	"alchemy_nobottles",
	"alchemy_not_reg",
	"alchemy_pour",
	"alchemy_stage_1",
	"alchemy_stage_2",
	"alchemy_toss",
	"anatomy_dex_1",
	"anatomy_dex_10",
	"anatomy_dex_2",
	"anatomy_dex_3",
	"anatomy_dex_4",
	"anatomy_dex_5",
	"anatomy_dex_6",
	"anatomy_dex_7",
	"anatomy_dex_8",
	"anatomy_dex_9",
	"anatomy_magic",
	"anatomy_result",
	"anatomy_str_1",
	"anatomy_str_10",
	"anatomy_str_2",
	"anatomy_str_3",
	"anatomy_str_4",
	"anatomy_str_5",
	"anatomy_str_6",
	"anatomy_str_7",
	"anatomy_str_8",
	"anatomy_str_9",
	"animallore_conjured",
	"animallore_food",
	"animallore_free",
	"animallore_master",
	"animallore_master_you",
	"animallore_result",
	"armslore_dam",
	"armslore_dam_1",
	"armslore_dam_10",
	"armslore_dam_2",
	"armslore_dam_3",
	"armslore_dam_4",
	"armslore_dam_5",
	"armslore_dam_6",
	"armslore_dam_7",
	"armslore_dam_8",
	"armslore_dam_9",
	"armslore_def",
	"armslore_def_1",
	"armslore_def_10",
	"armslore_def_2",
	"armslore_def_3",
	"armslore_def_4",
	"armslore_def_5",
	"armslore_def_6",
	"armslore_def_7",
	"armslore_def_8",
	"armslore_def_9",
	"armslore_psn_1",
	"armslore_psn_10",
	"armslore_psn_2",
	"armslore_psn_3",
	"armslore_psn_4",
	"armslore_psn_5",
	"armslore_psn_6",
	"armslore_psn_7",
	"armslore_psn_8",
	"armslore_psn_9",
	"armslore_rep",
	"armslore_rep_0",
	"armslore_unable",
	"begging_start",
	"bvbox_full_items",
	"bvbox_full_weight",
	"bvbox_open_other",
	"bvbox_open_self",
	"cant_make",
	"cant_make_res",
	"cartography_cant",
	"cartography_fail",
	"cartography_nomap",
	"cartography_wmap",
	"carve_corpse_1",
	"carve_corpse_2",
	"carve_corpse_feathers",
	"carve_corpse_fur",
	"carve_corpse_hides",
	"carve_corpse_meat",
	"carve_corpse_wool",
	"cmd_invalid",
	"cmdafk_enter",
	"cmdafk_leave",
	"combat_arch_noammo",
	"combat_arch_tooclose",
	"combat_attacko",
	"combat_attacks",
	"combat_hit_arm1o",
	"combat_hit_arm1s",
	"combat_hit_arm2o",
	"combat_hit_arm2s",
	"combat_hit_arm3o",
	"combat_hit_arm3s",
	"combat_hit_back1o",
	"combat_hit_back1s",
	"combat_hit_chest1o",
	"combat_hit_chest1s",
	"combat_hit_chest2o",
	"combat_hit_chest2s",
	"combat_hit_chest3o",
	"combat_hit_chest3s",
	"combat_hit_chest4o",
	"combat_hit_chest4s",
	"combat_hit_chest5o",
	"combat_hit_chest5s",
	"combat_hit_chest6o",
	"combat_hit_chest6s",
	"combat_hit_feet1o",
	"combat_hit_feet1s",
	"combat_hit_hand1o",
	"combat_hit_hand1s",
	"combat_hit_hand2o",
	"combat_hit_hand2s",
	"combat_hit_hand3o",
	"combat_hit_hand3s",
	"combat_hit_head1o",
	"combat_hit_head1s",
	"combat_hit_head2o",
	"combat_hit_head2s",
	"combat_hit_head3o",
	"combat_hit_head3s",
	"combat_hit_head4o",
	"combat_hit_head4s",
	"combat_hit_head5o",
	"combat_hit_head5s",
	"combat_hit_head6o",
	"combat_hit_head6s",
	"combat_hit_legs1o",
	"combat_hit_legs1s",
	"combat_hit_legs2o",
	"combat_hit_legs2s",
	"combat_hit_legs3o",
	"combat_hit_legs3s",
	"combat_hit_neck1o",
	"combat_hit_neck1s",
	"combat_hit_neck2o",
	"combat_hit_neck2s",
	"combat_misso",
	"combat_misss",
	"combat_parry",
	"cont_full",
	"cont_magic",
	"cont_toosmall",
	"cooking_success",
	"corpse_name",
	"detecthidden_succ",
	"drink_cantmove",
	"drink_potion_delay",
	"evalint_int_1",
	"evalint_int_10",
	"evalint_int_2",
	"evalint_int_3",
	"evalint_int_4",
	"evalint_int_5",
	"evalint_int_6",
	"evalint_int_7",
	"evalint_int_8",
	"evalint_int_9",
	"evalint_mag_1",
	"evalint_mag_2",
	"evalint_mag_3",
	"evalint_mag_4",
	"evalint_mag_5",
	"evalint_mag_6",
	"evalint_man_1",
	"evalint_man_2",
	"evalint_man_3",
	"evalint_man_4",
	"evalint_man_5",
	"evalint_man_6",
	"evalint_result",
	"evalint_result_2",
	"fishing_1",
	"fishing_2",
	"fishing_3",
	"fishing_promt",
	"fishing_reach",
	"fishing_success",
	"food_canteat",
	"food_canteatf",
	"food_cantmove",
	"food_full_1",
	"food_full_2",
	"food_full_3",
	"food_full_4",
	"food_full_5",
	"food_full_6",
	"food_rcanteat",
	"forensics_alive",
	"forensics_carve_1",
	"forensics_carve_2",
	"forensics_corpse",
	"forensics_failname",
	"forensics_name",
	"forensics_reach",
	"forensics_timer",
	"gmpage_prompt",
	"grandmaster_mark",
	"healing_am",
	"healing_attempt",
	"healing_attemptf",
	"healing_beyond",
	"healing_corpseg",
	"healing_cure_1",
	"healing_cure_2",
	"healing_ghost",
	"healing_healthy",
	"healing_interrupt",
	"healing_noaids",
	"healing_nonchar",
	"healing_noneed",
	"healing_reach",
	"healing_res",
	"healing_self",
	"healing_to",
	"healing_toofar",
	"healing_who",
	"healing_witem",
	"herding_annoyed",
	"herding_ltarg",
	"herding_nocrook",
	"herding_success",
	"hiding_revealed",
	"hiding_stumble",
	"hiding_success",
	"hiding_toolit",
	"inscription_fail",
	"item_magic",
	"item_newbie",
	"item_repair",
	"itemid_gold",
	"itemid_madeof",
	"itemid_noval",
	"itemid_result",
	"itemuse_advgate_fail",
	"itemuse_advgate_no",
	"itemuse_archb_block",
	"itemuse_archb_dest",
	"itemuse_archb_empty",
	"itemuse_archb_hit_1",
	"itemuse_archb_hit_2",
	"itemuse_archb_hit_3",
	"itemuse_archb_hit_4",
	"itemuse_archb_miss_1",
	"itemuse_archb_miss_2",
	"itemuse_archb_miss_3",
	"itemuse_archb_miss_4",
	"itemuse_archb_noammo",
	"itemuse_archb_p",
	"itemuse_archb_rem",
	"itemuse_archb_rems",
	"itemuse_archb_skill",
	"itemuse_archb_split",
	"itemuse_archb_ws",
	"itemuse_archb_x",
	"itemuse_bandage_clean",
	"itemuse_bandage_promt",
	"itemuse_bandage_reach",
	"itemuse_bboard_cor",
	"itemuse_bboard_del",
	"itemuse_bboard_reach",
	"itemuse_bedroll",
	"itemuse_beehive",
	"itemuse_beehive_sting",
	"itemuse_bolt_1",
	"itemuse_bolt_2",
	"itemuse_bolt_3",
	"itemuse_bolt_4",
	"itemuse_bolt_5",
	"itemuse_book_fail",
	"itemuse_bow_shield",
	"itemuse_cannon_empty",
	"itemuse_cannon_hpowder",
	"itemuse_cannon_hshot",
	"itemuse_cannon_lpowder",
	"itemuse_cannon_lshot",
	"itemuse_cannon_powder",
	"itemuse_cannon_shot",
	"itemuse_cannon_targ",
	"itemuse_cantthink",
	"itemuse_cball_promt",
	"itemuse_cotton_create",
	"itemuse_crook_promt",
	"itemuse_crook_try",
	"itemuse_dye_fail",
	"itemuse_dye_nohair",
	"itemuse_dye_reach",
	"itemuse_dye_targ",
	"itemuse_dye_unable",
	"itemuse_dye_vat",
	"itemuse_fish_fail",
	"itemuse_fish_unable",
	"itemuse_foodraw_promt",
	"itemuse_foodraw_touch",
	"itemuse_foodraw_use",
	"itemuse_forge",
	"itemuse_gameboard_fail",
	"itemuse_guildstone_new",
	"itemuse_hatch_fail",
	"itemuse_junk_reach",
	"itemuse_key_fail",
	"itemuse_key_nokey",
	"itemuse_key_nolock",
	"itemuse_key_promt",
	"itemuse_kindling_cont",
	"itemuse_kindling_fail",
	"itemuse_locked",
	"itemuse_log_unable",
	"itemuse_log_use",
	"itemuse_loom",
	"itemuse_loom_remove",
	"itemuse_macepick_targ",
	"itemuse_map_fail",
	"itemuse_mortar_promt",
	"itemuse_multi_blocked",
	"itemuse_multi_bump",
	"itemuse_multi_collapse",
	"itemuse_multi_fail",
	"itemuse_multi_intway",
	"itemuse_multi_shipw",
	"itemuse_no_mortar",
	"itemuse_pdummy_ok",
	"itemuse_pdummy_p",
	"itemuse_pdummy_skill",
	"itemuse_pitcher_fill",
	"itemuse_pitcher_reach",
	"itemuse_pitcher_targ",
	"itemuse_port_locked",
	"itemuse_potion_fail",
	"itemuse_rune_name",
	"itemuse_scissors_use",
	"itemuse_sewkit_promt",
	"itemuse_sextant",
	"itemuse_sextant_t2a",
	"itemuse_shipside",
	"itemuse_shrine",
	"itemuse_skit_unable",
	"itemuse_spawnchar_neg",
	"itemuse_spawnchar_rset",
	"itemuse_spawnitem_trig",
	"itemuse_spinwheel",
	"itemuse_spyglass_fe",
	"itemuse_spyglass_m1",
	"itemuse_spyglass_m2",
	"itemuse_spyglass_m3",
	"itemuse_spyglass_m4",
	"itemuse_spyglass_m5",
	"itemuse_spyglass_m6",
	"itemuse_spyglass_m7",
	"itemuse_spyglass_m8",
	"itemuse_spyglass_tr",
	"itemuse_steal",
	"itemuse_sweb_stuck",
	"itemuse_tdummy_arch",
	"itemuse_tdummy_p",
	"itemuse_tdummy_skill",
	"itemuse_telescope",
	"itemuse_tillerman",
	"itemuse_toofar",
	"itemuse_tracker_attune",
	"itemuse_trashcan",
	"itemuse_unable",
	"itemuse_weapon_immune",
	"itemuse_weapon_promt",
	"itemuse_weapon_wwait",
	"itemuse_wool_create",
	"lockpicking_nopick",
	"lockpicking_preach",
	"lockpicking_reach",
	"lockpicking_witem",
	"login_player",
	"login_players",
	"lumberjacking_1",
	"lumberjacking_2",
	"lumberjacking_3",
	"lumberjacking_4",
	"lumberjacking_5",
	"lumberjacking_6",
	"lumberjacking_los",
	"lumberjacking_reach",
	"magery_1",
	"magery_2",
	"magery_3",
	"magery_4",
	"makesuccess_1",
	"makesuccess_2",
	"makesuccess_3",
	"makesuccess_4",
	"makesuccess_5",
	"makesuccess_6",
	"meditation_peace_1",
	"meditation_peace_2",
	"meditation_try",
	"mining_1",
	"mining_2",
	"mining_3",
	"mining_4",
	"mining_cantuse",
	"mining_consumed",
	"mining_fire",
	"mining_forge",
	"mining_ingots",
	"mining_los",
	"mining_not_ore",
	"mining_nothing",
	"mining_reach",
	"mining_skill",
	"mining_tool",
	"msg_acc_alreadyu",
	"msg_acc_badpass",
	"msg_acc_block",
	"msg_acc_blocked",
	"msg_acc_denied",
	"msg_acc_email_fail",
	"msg_acc_email_success",
	"msg_acc_gused",
	"msg_acc_kick",
	"msg_acc_needpass",
	"msg_acc_priv",
	"msg_acc_unk",
	"msg_acc_wcli",
	"msg_alreadyonline",
	"msg_arrdep_1",
	"msg_arrdep_2",
	"msg_arrdep_3",
	"msg_bounce_pack",
	"msg_cantpush",
	"msg_cantsleep",
	"msg_cmd_lackpriv",
	"msg_corpse_of",
	"msg_coward_1",
	"msg_coward_2",
	"msg_criminal",
	"msg_eatsome",
	"msg_email_failed",
	"msg_email_notset_1",
	"msg_email_notset_2",
	"msg_emote_1",
	"msg_emote_2",
	"msg_emote_3",
	"msg_emote_4",
	"msg_emote_5",
	"msg_emote_6",
	"msg_emote_7",
	"msg_err_invset",
	"msg_err_noblankring",
	"msg_err_not4sale",
	"msg_err_notgamepiece",
	"msg_err_notkey",
	"msg_fatigue",
	"msg_feet",
	"msg_figurine_notyours",
	"msg_follow_arrow",
	"msg_food_lvl_1",
	"msg_food_lvl_2",
	"msg_food_lvl_3",
	"msg_food_lvl_4",
	"msg_food_lvl_5",
	"msg_food_lvl_6",
	"msg_food_lvl_7",
	"msg_food_lvl_8",
	"msg_forgiven",
	"msg_frozen",
	"msg_gmpage_canceled",
	"msg_gmpage_notified",
	"msg_gmpage_qnum",
	"msg_gmpage_qued",
	"msg_gmpage_rec",
	"msg_gmpage_update",
	"msg_gmpages",
	"msg_guards",
	"msg_guest",
	"msg_guildresign",
	"msg_heavy",
	"msg_hunger",
	"msg_invisible",
	"msg_it_is_dead",
	"msg_itemplace",
	"msg_jailed",
	"msg_key_blanks",
	"msg_key_cantreach",
	"msg_key_doorlocked",
	"msg_key_failc",
	"msg_key_isblank",
	"msg_key_nolock",
	"msg_key_notblanks",
	"msg_key_setname",
	"msg_key_targ",
	"msg_key_targ_cont_lock",
	"msg_key_targ_cont_ulock",
	"msg_key_targ_door_lock",
	"msg_key_targ_door_ulock",
	"msg_key_targ_hold_lock",
	"msg_key_targ_hold_ulock",
	"msg_key_targ_nolock",
	"msg_key_targ_reach",
	"msg_key_targ_ship_lock",
	"msg_key_targ_ship_ulock",
	"msg_key_targ_sign",
	"msg_key_wronglock",
	"msg_killed_by",
	"msg_magic_block",
	"msg_mailbag_drop_1",
	"msg_mailbag_drop_2",
	"msg_maxchars",
	"msg_mount_ceiling",
	"msg_mount_dist",
	"msg_mount_dontown",
	"msg_mount_unable",
	"msg_murderer",
	"msg_nospellbook",
	"msg_noto_change_0",
	"msg_noto_change_1",
	"msg_noto_change_2",
	"msg_noto_change_3",
	"msg_noto_change_4",
	"msg_noto_change_5",
	"msg_noto_change_6",
	"msg_noto_change_7",
	"msg_noto_change_8",
	"msg_noto_change_gain",
	"msg_noto_change_lost",
	"msg_noto_gettitle",
	"msg_overload",
	"msg_pet_food_1",
	"msg_pet_food_2",
	"msg_pet_food_3",
	"msg_pet_food_4",
	"msg_pet_food_5",
	"msg_pet_food_6",
	"msg_pet_food_7",
	"msg_pet_food_8",
	"msg_pet_happy_1",
	"msg_pet_happy_2",
	"msg_pet_happy_3",
	"msg_pet_happy_4",
	"msg_pet_happy_5",
	"msg_pet_happy_6",
	"msg_pet_happy_7",
	"msg_pet_happy_8",
	"msg_push",
	"msg_region_enter",
	"msg_region_guard_art",
	"msg_region_guards_1",
	"msg_region_guards_2",
	"msg_region_guardsp",
	"msg_region_guardspt",
	"msg_region_pvpnot",
	"msg_region_pvpsafe",
	"msg_region_safetyget",
	"msg_region_safetylose",
	"msg_region_water_1",
	"msg_region_water_2",
	"msg_rename_cancel",
	"msg_rename_success",
	"msg_rename_wname",
	"msg_seed_atree",
	"msg_seed_nogood",
	"msg_seed_reach",
	"msg_seed_targsoil",
	"msg_serv_ao",
	"msg_serv_full",
	"msg_serv_ld",
	"msg_shipname_promt",
	"msg_steal",
	"msg_stepon_body",
	"msg_stonereg_time",
	"msg_targ_link",
	"msg_targ_link_same",
	"msg_targ_link2",
	"msg_targ_links",
	"msg_targ_pc",
	"msg_tingling",
	"msg_toofar_vendor",
	"msg_trade_toofar",
	"msg_trade_wait",
	"msg_unconscious",
	"msg_wrestling_nogo",
	"msg_younotice_1",
	"msg_younotice_2",
	"msg_younotice_s",
	"msg_younotice_your",
	"multi_lockdown",
	"multi_lockup",
	"musicanship_notool",
	"musicanship_poor",
	"ncp_vendor_guards",
	"ncp_vendor_sell_ty",
	"non_alive",
	"npc_banker_deposit",
	"npc_beggar_beg_1",
	"npc_beggar_beg_2",
	"npc_beggar_beg_3",
	"npc_beggar_beg_4",
	"npc_beggar_beg_5",
	"npc_beggar_beg_6",
	"npc_beggar_food_tal",
	"npc_beggar_food_ty",
	"npc_beggar_ifonly",
	"npc_beggar_sell",
	"npc_generic_crim",
	"npc_generic_dontwant",
	"npc_generic_gone_1",
	"npc_generic_gone_2",
	"npc_generic_interrupt",
	"npc_generic_seecrim",
	"npc_generic_seemons",
	"npc_generic_snooped_1",
	"npc_generic_snooped_2",
	"npc_generic_snooped_3",
	"npc_generic_snooped_4",
	"npc_generic_thanks",
	"npc_guard_strike_1",
	"npc_guard_strike_2",
	"npc_guard_strike_3",
	"npc_guard_strike_4",
	"npc_guard_strike_5",
	"npc_guard_threat_1",
	"npc_guard_threat_2",
	"npc_guard_threat_3",
	"npc_guard_threat_4",
	"npc_guard_threat_5",
	"npc_healer_fail_1",
	"npc_healer_fail_2",
	"npc_healer_manifest",
	"npc_healer_range",
	"npc_healer_ref_crim_1",
	"npc_healer_ref_crim_2",
	"npc_healer_ref_crim_3",
	"npc_healer_ref_evil_1",
	"npc_healer_ref_evil_2",
	"npc_healer_ref_evil_3",
	"npc_healer_ref_good_1",
	"npc_healer_ref_good_2",
	"npc_healer_ref_good_3",
	"npc_healer_res_1",
	"npc_healer_res_2",
	"npc_healer_res_3",
	"npc_healer_res_4",
	"npc_healer_res_5",
	"npc_pet_cantsell",
	"npc_pet_carrynothing",
	"npc_pet_days_left",
	"npc_pet_decide_master",
	"npc_pet_deserted",
	"npc_pet_drop",
	"npc_pet_employed",
	"npc_pet_failure",
	"npc_pet_food_no",
	"npc_pet_food_ty",
	"npc_pet_getgold_1",
	"npc_pet_getgold_2",
	"npc_pet_hire_amnt",
	"npc_pet_hire_rate",
	"npc_pet_hire_time",
	"npc_pet_hire_timeup",
	"npc_pet_inv_only",
	"npc_pet_items_buy",
	"npc_pet_items_sample",
	"npc_pet_items_sell",
	"npc_pet_money",
	"npc_pet_not_enough",
	"npc_pet_not_for_hire",
	"npc_pet_not_work",
	"npc_pet_sell",
	"npc_pet_setprice",
	"npc_pet_success",
	"npc_pet_targ_att",
	"npc_pet_targ_fetch",
	"npc_pet_targ_follow",
	"npc_pet_targ_friend",
	"npc_pet_targ_go",
	"npc_pet_targ_guard",
	"npc_pet_targ_transfer",
	"npc_pet_thanks",
	"npc_pet_wage_cost",
	"npc_pet_weak",
	"npc_stablemaster_fail",
	"npc_stablemaster_full",
	"npc_stablemaster_los",
	"npc_stablemaster_nopets",
	"npc_stablemaster_rem",
	"npc_stablemaster_select",
	"npc_stablemaster_targ_stable",
	"npc_stablemaster_toomany",
	"npc_stablemaster_treatwell",
	"npc_text_murd_1",
	"npc_text_murd_2",
	"npc_text_murd_3",
	"npc_text_murd_4",
	"npc_trainer_dunno_1",
	"npc_trainer_dunno_2",
	"npc_trainer_dunno_3",
	"npc_trainer_dunno_4",
	"npc_trainer_enemy",
	"npc_trainer_forgot",
	"npc_trainer_price",
	"npc_trainer_price_1",
	"npc_trainer_price_2",
	"npc_trainer_price_3",
	"npc_trainer_success",
	"npc_trainer_thatsall_1",
	"npc_trainer_thatsall_2",
	"npc_trainer_thatsall_3",
	"npc_trainer_thatsall_4",
	"npc_vendor_b1",
	"npc_vendor_ca",
	"npc_vendor_cantafford",
	"npc_vendor_hyare",
	"npc_vendor_no_goods",
	"npc_vendor_nomoney",
	"npc_vendor_nothing_buy",
	"npc_vendor_nothing_sell",
	"npc_vendor_offduty",
	"npc_vendor_s1",
	"npc_vendor_sell",
	"npc_vendor_setprice_1",
	"npc_vendor_setprice_2",
	"npc_vendor_stat_gold_1",
	"npc_vendor_stat_gold_2",
	"npc_vendor_stat_gold_3",
	"npc_vendor_stat_gold_4",
	"npc_vendor_ty",
	"party_added",
	"party_disbanded",
	"party_invite",
	"party_invite_targ",
	"party_joined",
	"party_leave_1",
	"party_leave_2",
	"party_loot_allow",
	"party_loot_block",
	"party_part_1",
	"party_part_2",
	"party_select",
	"party_targ_who",
	"poisoning_select_1",
	"poisoning_success",
	"poisoning_witem",
	"provocation_emote_1",
	"provocation_emote_2",
	"provocation_player",
	"provocation_upset",
	"provoke_select",
	"provoke_unable",
	"reach_fail",
	"reach_ghost",
	"reach_unable",
	"removetraps_reach",
	"removetraps_witem",
	"repair_1",
	"repair_2",
	"repair_3",
	"repair_4",
	"repair_5",
	"repair_anvil",
	"repair_full",
	"repair_lack_1",
	"repair_lack_2",
	"repair_msg",
	"repair_not",
	"repair_reach",
	"repair_unk",
	"repair_worn",
	"server_resync_failed",
	"server_resync_start",
	"server_resync_success",
	"server_worldsave",
	"skill_noskill",
	"skillwait_1",
	"skillwait_2",
	"skillwait_3",
	"sleep_awake_1",
	"sleep_awake_2",
	"smithing_fail",
	"smithing_forge",
	"smithing_hammer",
	"smithing_reach",
	"snooping_mark",
	"snooping_reach",
	"snooping_someone",
	"snooping_your",
	"spell_alcohol_hic",
	"spell_animdead_fail",
	"spell_animdead_nc",
	"spell_cursed_magic",
	"spell_dispellf_wt",
	"spell_enchant_activate",
	"spell_enchant_lack",
	"spell_gate_am",
	"spell_gen_fizzles",
	"spell_looks",
	"spell_poison_1",
	"spell_poison_2",
	"spell_poison_3",
	"spell_poison_4",
	"spell_recall_blank",
	"spell_recall_fadec",
	"spell_recall_faded",
	"spell_recall_notrune",
	"spell_recall_sfade",
	"spell_res_am",
	"spell_res_rejoin",
	"spell_res_robename",
	"spell_targ_cont",
	"spell_targ_fieldc",
	"spell_targ_los",
	"spell_targ_noeffect",
	"spell_targ_obj",
	"spell_tele_am",
	"spell_tele_cant",
	"spell_tele_jailed_1",
	"spell_tele_jailed_2",
	"spell_try_am",
	"spell_try_dead",
	"spell_try_nobook",
	"spell_try_nomana",
	"spell_try_noregs",
	"spell_try_notyourbook",
	"spell_try_thereg",
	"spell_wand_nocharge",
	"spell_youfeel",
	"spiritspeak_success",
	"stealing_corpse",
	"stealing_empty",
	"stealing_gameboard",
	"stealing_heavy",
	"stealing_mark",
	"stealing_noneed",
	"stealing_nothing",
	"stealing_pickpocket",
	"stealing_reach",
	"stealing_safe",
	"stealing_someone",
	"stealing_stop",
	"stealing_trade",
	"stealing_your",
	"taming_1",
	"taming_2",
	"taming_3",
	"taming_4",
	"taming_cant",
	"taming_los",
	"taming_reach",
	"taming_remember",
	"taming_success",
	"taming_tame",
	"taming_tamed",
	"taming_ymaster",
	"target_promt",
	"tasteid_char",
	"tasteid_result",
	"tasteid_self",
	"tasteid_unable",
	"tracking_dist_0",
	"tracking_dist_1",
	"tracking_dist_2",
	"tracking_dist_3",
	"tracking_dist_4",
	"tracking_fail_1",
	"tracking_fail_2",
	"tracking_fail_3",
	"tracking_fail_4",
	"tracking_fail_5",
	"tracking_fail_6",
	"tracking_spyglass",
	"tracking_unable",
	// additional messages will be added here in unsorted fashion
	NULL,
};

DWORD ahextoi( LPCTSTR pszStr ) // Convert hex string to integer
{
	// Unfortunatly the library func cant handle the number FFFFFFFF
	// TCHAR * sstop; return( strtol( s, &sstop, 16 ));

	if ( pszStr == NULL )
		return( 0 );

	GETNONWHITESPACE( pszStr );

	DWORD val = 0;
	while (true)
	{
		TCHAR ch = *pszStr;
		if ( isdigit( ch ))
			ch -= '0';
		else
		{
			ch = toupper(ch);
			if ( ch > 'F' || ch <'A' )
				break;
			ch -= 'A' - 10;
		}
		val *= 0x10;
		val += ch;
		pszStr ++;
	}
	return( val );
}


bool IsStrEmpty( LPCTSTR pszTest )
{
	if ( !pszTest || !*pszTest ) return true;

	do
	{
		if ( !isspace(*pszTest) ) return false;
	}
	while( *(++pszTest) );
	return true;
}

bool IsStrNumericDec( LPCTSTR pszTest )
{
	if ( !pszTest || !*pszTest ) return false;

	do
	{
		if ( !isdigit(*pszTest) ) return false;
	}
	while ( *(++pszTest) );

	return true;
}


bool IsStrNumeric( LPCTSTR pszTest )
{
	if ( !pszTest || !*pszTest )
		return false;
	bool	fHex	= false;
	if ( pszTest[0] == '0' )
		fHex	= true;

	do
	{
		if ( isdigit( *pszTest ) )
			continue;
		if ( fHex && tolower(*pszTest) >= 'A' && tolower(*pszTest) <= 'F' )
			continue;
		return false;
	}
	while ( *(++pszTest) );
	return true;
}


bool IsSimpleNumberString( LPCTSTR pszTest )
{
	// is this a string or a simple numeric expression ?
	// string = 1 2 3, sdf, sdf sdf sdf, 123d, 123 d,
	// number = 1.0+-\*~|&!%^()2, 0aed, 123

	bool fMathSep = true;	// last non whitespace was a math sep.
	bool fHextDigitStart = false;
	bool fWhiteSpace = false;

	for ( ; true; pszTest++ )
	{
		TCHAR ch = *pszTest;
		if ( ! ch )
		{
			return( true );
		}
		if (( ch >= 'A' && ch <= 'F') || ( ch >= 'a' && ch <= 'f' ))	// isxdigit
		{
			if ( ! fHextDigitStart )
				return( false );
			fWhiteSpace = false;
			fMathSep = false;
			continue;
		}
		if ( isspace( ch ))
		{
			fHextDigitStart = false;
			fWhiteSpace = true;
			continue;
		}
		if ( isdigit( ch ))
		{
			if ( fWhiteSpace && ! fMathSep )
				return false;
			if ( ch == '0' )
			{
				fHextDigitStart = true;
			}
			fWhiteSpace = false;
			fMathSep = false;
			continue;
		}
		if ( ch == '/' && pszTest[1] != '/' )
			fMathSep	= true;
		else
			fMathSep = strchr("+-\\*~|&!%^()", ch ) ? true : false ;

		if ( ! fMathSep )
		{
			return( false );
		}
		fHextDigitStart = false;
		fWhiteSpace = false;
	}
}

static int GetIdentifierString( TCHAR * szTag, LPCTSTR pszArgs )
{
	// Copy the identifier (valid char set) out to this buffer.
	int i=0;
	for ( ;pszArgs[i]; i++ )
	{
		if ( ! _ISCSYM(pszArgs[i]))
			break;
		if ( i>=EXPRESSION_MAX_KEY_LEN )
			return( NULL );
		szTag[i] = pszArgs[i];
	}

	szTag[i] = '\0';
	return i;
}

/////////////////////////////////////////////////////////////////////////
// -CVarDefArray

int CVarDefArray::FindValStr( LPCTSTR pVal ) const
{
	int iQty = GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		const CVarDefBase * pVarBase = GetAt(i);
		ASSERT( pVarBase );
		const CVarDefStr * pVarStr = dynamic_cast <const CVarDefStr *>( pVarBase );
		if ( pVarStr == NULL )
			continue;
		if ( ! strcmpi( pVal, pVarStr->GetValStr()))
			return( i );
	}
	return( -1 );
}

int CVarDefArray::FindValNum( int iVal ) const
{
	// Find a value in the array. (Not a name)
	// just used for testing purposes.

	int iQty = GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		const CVarDefBase * pVarBase = GetAt(i);
		ASSERT( pVarBase );
		const CVarDefNum * pVarNum = dynamic_cast <const CVarDefNum *>( pVarBase );
		if ( pVarNum == NULL )
			continue;
		int iValCmp = pVarNum->GetValNum();
		if ( iValCmp == iVal )
			return( i );
	}
	return( -1 );
}

void CVarDefArray::Copy( const CVarDefArray * pArray )
{
	if ( this == pArray )
		return;
	Empty();
	int iQty = pArray->GetCount();
	SetCount( iQty );
	for ( int i=0; i<iQty; i++ )
	{
		SetAt( i, pArray->GetAt(i)->CopySelf() );
	}
}

int CVarDefArray::SetNumNew( LPCTSTR pszName, int iVal )
{
	// This is dangerous , we know it to be a new entry !
	// Not bothering to check for duplication.
	// ASSUME we already did FindKey

	CVarDefBase * pVarNum = new CVarDefNum( pszName, iVal );
	ASSERT(pVarNum);
	return Add( pVarNum );
}

int CVarDefArray::SetNum( LPCTSTR pszName, int iVal, bool fZero )
{
	ASSERT(pszName);
	if ( pszName[0] == '\0' )
		return( -1 );


	if ( fZero && (iVal == 0) )	// but not if empty
	{
		int i = FindKey( pszName );
		if ( i >= 0 )
		{
			DeleteAt(i);
		}
		return( -1 );
	}

	// Create if necessary.
	int i = FindKey( pszName );
	if ( i < 0 )
	{
		// key does not exist so create it.
		return SetNumNew( pszName, iVal );
	}

	// replace existing key.

	CVarDefBase * pVarBase = GetAt(i);
	ASSERT(pVarBase);

	CVarDefNum * pVarNum	= dynamic_cast <CVarDefNum *>( pVarBase );
	if ( pVarNum )
	{
		pVarNum->SetValNum( iVal );
	}
	else
	{
		// Replace the existing.
		// This is dangerous !!!
		if ( g_Serv.IsLoading() )
		{
			DEBUG_ERR(( "Replace existing VarStr '%s'\n", pVarBase->GetKey()));
		}
		SetAt( i, new CVarDefNum( pszName, iVal ));
	}
	return( i );
}

int CVarDefArray::SetStr( LPCTSTR pszName, bool fQuoted, LPCTSTR pszVal, bool fZero )
{
	// ASSUME: This has been clipped of unwanted beginning and trailing spaces.
	ASSERT(pszName);
	if ( pszName[0] == '\0' )
		return( -1 );

	if ( pszVal == NULL || pszVal[0] == '\0' )	// but not if empty
	{
		int i = FindKey( pszName );
		if ( i >= 0 )
		{
			DeleteAt(i);
		}
		return( -1 );
	}

	if ( !fQuoted && IsSimpleNumberString(pszVal))
	{
		// Just store the number and not the string.
		return SetNum( pszName, Exp_GetVal( pszVal ));
	}

	// Create if necessary.
	int i = FindKey( pszName );
	if ( i < 0 )
	{
		// key does not exist so create it.
		return Add( new CVarDefStr( pszName, pszVal ) );
	}

	// replace existing key.
	CVarDefBase * pVarBase = GetAt(i);
	ASSERT(pVarBase);

	CVarDefStr * pVarStr = dynamic_cast <CVarDefStr *>( pVarBase );
	if ( pVarStr )
	{
		pVarStr->SetValStr( pszVal );
	}
	else
	{
		// Replace the existing. (this will kill crash any existing pointers !)
		if ( g_Serv.IsLoading())
		{
			DEBUG_ERR(( "Replace existing VarNum '%s' with %s\n", pVarBase->GetKey(), pszVal ));
		}
		SetAt( i, new CVarDefStr( pszName, pszVal ));
	}
	return( i );
}

CVarDefBase * CVarDefArray::GetKey( LPCTSTR pszKey ) const
{
	if ( pszKey )
	{
		int j = FindKey( pszKey );
		if ( j >= 0 )
		{
			return( GetAt(j));
		}
	}
	return( NULL );
}

LPCTSTR CVarDefArray::GetKeyStr( LPCTSTR pszKey, bool fZero  ) const
{
	CVarDefBase * pVar = GetKey(pszKey);
	if ( pVar == NULL )
		return (fZero ? "0" : "");
	return pVar->GetValStr();
}

CVarDefBase * CVarDefArray::GetParseKey( LPCTSTR & pszArgs ) const
{
	// Skip to the end of the expression name.
	// The name can only be valid.

	TCHAR szTag[ EXPRESSION_MAX_KEY_LEN ];
	int i = GetIdentifierString( szTag, pszArgs );
	int j = FindKey( szTag );
	if ( j >= 0 )
	{
		pszArgs += i;
		return( GetAt(j));
	}

	if ( this == (&g_Exp.m_VarGlobals) )
		return NULL;

	g_pLog->Event( LOGL_TRACE, "WARNING: can't find definition for '%s'!\n", pszArgs );
	return NULL;
}

bool CVarDefArray::GetParseVal( LPCTSTR & pszArgs, long * plVal ) const
{
	// Find the key in the defs collection

	CVarDefBase * pVarBase = GetParseKey( pszArgs );
	if ( pVarBase == NULL )
		return( false );
	*plVal = pVarBase->GetValNum();
	return( true );
}

void CVarDefArray::r_WriteTags( CScript & s )
{
	LPCTSTR		pszVal;
	// Write with the TAG. prefix.
	for ( int i=GetCount(); --i >= 0; )
	{
		CVarDefBase * pVar = GetAt(i);
		ASSERT(pVar);

		char	*z = Str_GetTemp();
		sprintf(z, "TAG.%s", pVar->GetKey());
		pszVal = pVar->GetValStr();
		CVarDefStr * pVarStr = dynamic_cast <CVarDefStr *>(pVar);
		if ( pVarStr /*isspace(pszVal[0]) || isspace( pszVal[strlen(pszVal)-1] )*/ )
			s.WriteKeyFormat(z, "\"%s\"", pszVal);
		else
			s.WriteKey(z, pszVal);
	}
}

void CVarDefArray::DumpKeys( CTextConsole * pSrc, LPCTSTR pszPrefix )
{
	// List out all the keys.
	ASSERT(pSrc);
	if ( pszPrefix == NULL )
		pszPrefix = "";
	int iQty = GetCount();
	for ( int i=0; i<iQty; i++ )
	{
		const CVarDefBase * pVar = GetAt(i);
		pSrc->SysMessagef( "%s%s=%s", (LPCTSTR) pszPrefix, (LPCTSTR) pVar->GetKey(), (LPCTSTR) pVar->GetValStr());
	}
}

void CVarDefArray::ClearKeys(LPCTSTR mask)
{
	// Clear the tag array
	for ( int i=GetCount(); --i >= 0;)
	{
		if ( mask && *mask )
		{
			CVarDefBase *pVar = GetAt(i);
			if ( !strstr(pVar->GetKey(), mask) ) continue;
		}
		DeleteAt(i);
	}
}


///////////////////////////////////////////////////////////////
// -CExpression

CExpression::CExpression()
{
}

CExpression::~CExpression()
{
}

enum INTRINSIC_TYPE
{
	INTRINSIC_ID = 0,
	INTRINSIC_ISBIT,
	INTRINSIC_ISNUMBER,
	INTRINSIC_QVAL,
	INTRINSIC_RAND,
	INTRINSIC_RANDBELL,
	INTRINSIC_STRASCII,
	INTRINSIC_STRCMP,
	INTRINSIC_STRCMPI,
	INTRINSIC_StrIndexOf,
	INTRINSIC_STRLEN,
	INTRINSIC_STRMATCH,
	INTRINSIC_QTY,
};

static LPCTSTR const sm_IntrinsicFunctions[INTRINSIC_QTY+1] =
{
	"ID",		// ID(x) = truncate the type portion of an Id
	"ISBIT",	// ISBIT(number,bit) = is the bit set, 1 or 0
	"ISNUMBER",		// ISNUMBER(var)
	"QVAL",		// QVAL(test1,test2,ret1,ret2,ret3) - test1 ? test2 (< ret1, = ret2, > ret3)
	"RAND",		// RAND(x) = flat random
	"RANDBELL",	// RANDBELL(center,variance25)
	"StrAscii",
	"STRCMP",	// STRCMP(str1,str2)
	"STRCMPI",	// STRCMPI(str1,str2)
	"StrIndexOf", // StrIndexOf(string,searchVal,[index]) = find the index of this, -1 = not here.
	"STRLEN",	// STRLEN(str)
	"STRMATCH",	// STRMATCH(str,*?pattern)
	NULL,
	// Others? SQRT(), SIN(), COS(), TAN(),
};

int CExpression::GetSingle( LPCTSTR & pszArgs )
{
	// Parse just a single expression without any operators or ranges.
	ASSERT(pszArgs);
	GETNONWHITESPACE( pszArgs );

	if ( pszArgs[0] == '0' )	// leading '0' = hex value.
	{
		// A hex value.
		if ( pszArgs[1] == '.' )	// leading 0. means it really is decimal.
		{
			pszArgs += 2;
			goto try_dec;
		}

		LPCTSTR pStart = pszArgs;
		DWORD val = 0;
		while ( true )
		{
			TCHAR ch = *pszArgs;
			if ( isdigit( ch ))
				ch -= '0';
			else
			{
				ch = tolower(ch);
				if ( ch > 'f' || ch <'a' )
				{
					if ( ch == '.' && pStart[0] != '0' )	// ok i'm confused. it must be decimal.
					{
						pszArgs = pStart;
						goto try_dec;
					}
					break;
				}
				ch -= 'a' - 10;
			}
			val *= 0x10;
			val += ch;
			pszArgs ++;
		}
		return( val );
	}
	else if ( pszArgs[0] == '.' || isdigit(pszArgs[0]))
	{
		// A decminal number
try_dec:
		long iVal = 0;
		for ( ;true; pszArgs++ )
		{
			if ( *pszArgs == '.' )
				continue;	// just skip this.
			if ( ! isdigit( *pszArgs ))
				break;
			iVal *= 10;
			iVal += *pszArgs - '0';
		}
		return( iVal );
	}
	else if ( ! _ISCSYMF(pszArgs[0]))
	{
		// some sort of math op ?

		switch ( pszArgs[0] )
		{
		case '{':
			pszArgs ++;
			return( GetRange( pszArgs ));
		case '[':
		case '(': // Parse out a sub expression.
			pszArgs ++;
			return( GetVal( pszArgs ));
		case '+':
			pszArgs++;
			break;
		case '-':
			pszArgs++;
			return( -GetSingle( pszArgs ));
		case '~':	// Bitwise not.
			pszArgs++;
			return( ~GetSingle( pszArgs ));
		case '!':	// boolean not.
			pszArgs++;
			if ( pszArgs[0] == '=' )  // odd condition such as (!=x) which is always true of course.
			{
				pszArgs++;		// so just skip it. and compare it to 0
				return( GetSingle( pszArgs ));
			}
			return( !GetSingle( pszArgs ));
		case ';':	// seperate feild.
		case ',':	// seperate feild.
		case '\0':
			return( 0 );
		}
	}
	else
	{
		// Symbol or intrinsinc function ?

		INTRINSIC_TYPE iIntrinsic = (INTRINSIC_TYPE) FindTableHeadSorted( pszArgs, sm_IntrinsicFunctions, COUNTOF(sm_IntrinsicFunctions)-1 );
		if ( iIntrinsic >= 0 )
		{
			int iLen = strlen(sm_IntrinsicFunctions[iIntrinsic]);
			if ( pszArgs[iLen] == '(' )
			{
			pszArgs += iLen+1;
			TCHAR * pszArgsNext;
			Str_Parse( const_cast<TCHAR*>(pszArgs), &(pszArgsNext), ")" );

			TCHAR * ppCmd[5];
			int iCount = Str_ParseCmds( const_cast<TCHAR*>(pszArgs), ppCmd, COUNTOF(ppCmd), "," );
			if ( ! iCount )
			{
				DEBUG_ERR(( "Bad intrinsic function usage. missing )\n" ));
				return 0;
			}

			pszArgs = pszArgsNext;

			switch ( iIntrinsic )
			{
			case INTRINSIC_ID:
				return( RES_GET_INDEX( GetVal( ppCmd[0] )));	// RES_GET_INDEX
			case INTRINSIC_StrIndexOf:
				// 2 or 3 args. ???
				return( -1 );
			case INTRINSIC_STRMATCH:
				if ( iCount < 2 )
					return( 0 );
				return(( Str_Match( ppCmd[0], ppCmd[1] ) == MATCH_VALID ) ? 1 : 0 );
			case INTRINSIC_RANDBELL:
				if ( iCount < 2 )
					return( 0 );
				return( Calc_GetBellCurve( GetVal( ppCmd[0] ), GetVal( ppCmd[1] )));
			case INTRINSIC_ISBIT:
				return( GetVal( ppCmd[0] ) & ( 1 << GetVal( ppCmd[1] )));
			case INTRINSIC_STRASCII:
				return( ppCmd[0][0] );
			case INTRINSIC_RAND:

				if ( iCount == 2 )
				{
					int		val1	= GetVal( ppCmd[0] );
					int		val2	= GetVal( ppCmd[1] );
					return val1 + Calc_GetRandVal( val2 - val1 );
				}
				else
					return( Calc_GetRandVal( GetVal( ppCmd[0] )));
			case INTRINSIC_STRCMP:
				if ( iCount < 2 )
					return 1;
				return strcmp(ppCmd[0], ppCmd[1]);
			case INTRINSIC_STRCMPI:
				if ( iCount < 2 )
					return( 1 );
				return( strcmpi( ppCmd[0], ppCmd[1] ));
			case INTRINSIC_STRLEN:
				return( strlen( ppCmd[0] ));
			case INTRINSIC_ISNUMBER:
				char z[64];
				LTOA(atol(ppCmd[0]), z, 10);
				return ( strcmp(ppCmd[0], z) ? 0 : 1 );
			case INTRINSIC_QVAL:
				if ( iCount < 3 ) return 0;
				int a1 = GetSingle(ppCmd[0]);
				int a2 = GetSingle(ppCmd[1]);
				if ( a1 < a2 ) return GetSingle(ppCmd[2]);
				else if ( a1 == a2 ) return ( iCount < 4 ) ? 0 : GetSingle(ppCmd[3]);
				else return ( iCount < 5 ) ? 0 : GetSingle(ppCmd[4]);
			}

			}
		}

		// Must be a symbol of some sort ?
		long lVal;
		if ( m_VarGlobals.GetParseVal( pszArgs, &lVal ) )
			return( lVal );
		if ( m_VarDefs.GetParseVal( pszArgs, &lVal ) )
			return( lVal );
	}

	// hard end ! Error of some sort.
	TCHAR szTag[ EXPRESSION_MAX_KEY_LEN ];
	int i = GetIdentifierString( szTag, pszArgs );
	pszArgs += i;	// skip it.

	DEBUG_ERR(( "Undefined symbol '%s'\n", szTag ));
	return( 0 );
}

int CExpression::GetValMath( int lVal, LPCTSTR & pExpr )
{
	GETNONWHITESPACE(pExpr);

	// Look for math type operator.
	switch ( pExpr[0] )
	{
	case '\0':
		break;
	case ')':  // expression end markers.
	case '}':
	case ']':
		pExpr++;	// consume this.
		break;
	case '+':
		pExpr++;
		lVal += GetVal( pExpr );
		break;
	case '-':
		pExpr++;
		lVal -= GetVal( pExpr );
		break;
	case '*':
		pExpr++;
		lVal *= GetVal( pExpr );
		break;
	case '|':
		pExpr++;
		if ( pExpr[0] == '|' )	// boolean ?
		{
			pExpr++;
			lVal = ( GetVal( pExpr ) || lVal );
		}
		else	// bitwise
		{
			lVal |= GetVal( pExpr );
		}
		break;
	case '&':
		pExpr++;
		if ( pExpr[0] == '&' )	// boolean ?
		{
			pExpr++;
			lVal = ( GetVal( pExpr ) && lVal );	// tricky stuff here. logical ops must come first or possibly not get processed.
		}
		else	// bitwise
		{
			lVal &= GetVal( pExpr );
		}
		break;
	case '/':
		pExpr++;
		{
			long iVal = GetVal( pExpr );
			if ( ! iVal )
			{
				DEBUG_ERR(( "Exp_GetVal Divide by 0\n" ));
				break;
			}
			lVal /= iVal;
		}
		break;
	case '%':
		pExpr++;
		{
			long iVal = GetVal( pExpr );
			if ( ! iVal )
			{
				DEBUG_ERR(( "Exp_GetVal Divide by 0\n" ));
				break;
			}
			lVal %= iVal;
		}
		break;
	case '>': // boolean
		pExpr++;
		if ( pExpr[0] == '=' )	// boolean ?
		{
			pExpr++;
			lVal = ( lVal >= GetVal( pExpr ));
		}
		else if ( pExpr[0] == '>' )	// boolean ?
		{
			pExpr++;
			lVal >>= GetVal( pExpr );
		}
		else
		{
			lVal = ( lVal > GetVal( pExpr ));
		}
		break;
	case '<': // boolean
		pExpr++;
		if ( pExpr[0] == '=' )	// boolean ?
		{
			pExpr++;
			lVal = ( lVal <= GetVal( pExpr ));
		}
		else if ( pExpr[0] == '<' )	// boolean ?
		{
			pExpr++;
			lVal <<= GetVal( pExpr );
		}
		else
		{
			lVal = ( lVal < GetVal( pExpr ));
		}
		break;
	case '!':
		pExpr ++;
		if ( pExpr[0] != '=' )
			break; // boolean ! is handled as a single expresion.
		pExpr ++;
		lVal = ( lVal != GetVal( pExpr ));
		break;
	case '=': // boolean
		while ( pExpr[0] == '=' )
			pExpr ++;
		lVal = ( lVal == GetVal( pExpr ));
		break;
	case '^':
		pExpr ++;
		lVal = ( lVal ^ GetVal( pExpr ));
		break;
	}

	return( lVal );
}

int CExpression::GetVal( LPCTSTR & pExpr )
{
	// Get a value (default decimal) that could also be an expression.
	// This does not parse beyond a comma !
	//
	// These are all the type of expressions and defines we'll see:
	//
	//	all_skin_colors				// simple DEF value
	//	7933 						// simple decimal
	//  -100.0						// simple negative decimal
	//  .5							// simple decimal
	//  0.5							// simple decimal
	//	073a 						// hex value (leading zero and no .)
	//
	//  0 -1					// Subtraction. has a space seperator. (Yes I know I hate this)
	//	{0-1}						// hyphenated simple range (GET RID OF THIS!)
	//				complex ranges must be in {}
	//	{ 3 6}						// simple range
	//	{ 400 1 401 1 } 				// weighted values (2nd val = 1)
	//	{ 1102 1148 1 }				// weighted range (3rd val < 10)
	//	{ animal_colors 1 no_colors 1 } 	// weighted range
	//	{ red_colors 1 {34 39} 1 }		// same (red_colors expands to a range)

	if ( pExpr == NULL )
		return( 0 );

	GETNONWHITESPACE( pExpr );

	int lVal = GetSingle( pExpr );

	return GetValMath( lVal, pExpr );
}

int CExpression::GetRangeVals( LPCTSTR & pExpr, int * piVals, int iMaxQty )
{
	// Get a list of values.
	if ( pExpr == NULL )
		return( 0 );

	ASSERT(piVals);

	int iQty = 0;
	while (true)
	{
		if ( !pExpr[0] ) break;
		if ( pExpr[0] == ';' )
			break;	// seperate field.
		if ( pExpr[0] == ',' )
			pExpr++;

		piVals[iQty] = GetSingle( pExpr );
		if ( ++iQty >= iMaxQty )
			break;
		if ( pExpr[0] == '-' && iQty == 1 )	// range seperator. (if directly after, I know this is sort of strange)
		{
			pExpr++;	// ??? This is stupid. get rid of this and clean up it's use in the scripts.
			continue;
		}

		GETNONWHITESPACE(pExpr);

		// Look for math type operator.
		switch ( pExpr[0] )
		{
		case ')':  // expression end markers.
		case '}':
		case ']':
			pExpr++;	// consume this and end.
			return( iQty );

		case '+':
		case '*':
		case '/':
		case '%':
		case '<':
		case '>':
		case '|':
		case '&':
			piVals[iQty-1] = GetValMath( piVals[iQty-1], pExpr );
			break;
		}
	}

	return( iQty );
}

int CExpression::GetRange( LPCTSTR & pExpr )
{
	int lVals[64];		// Maximum elements in a list

	int iQty = GetRangeVals( pExpr, lVals, COUNTOF(lVals));

	if (iQty == 0)
	{
		return( 0 );
	}
	if (iQty == 1) // It's just a simple value
	{
		return( lVals[0] );
	}
	if (iQty == 2) // It's just a simple range....pick one in range at random
	{
		return( min( lVals[0], lVals[1] ) + Calc_GetRandVal( abs( lVals[1] - lVals[0] ) + 1 ));
	}

	// I guess it's weighted values
	// First get the total of the weights

	int iTotalWeight = 0;
	int i = 1;
	for ( ; i < iQty; i+=2 )
	{
		if ( ! lVals[i] )	// having a weight of 0 is very strange !
		{
			DEBUG_ERR(( "Weight of 0 in random set?\n" ));	// the whole table should really just be invalid here !
		}
		iTotalWeight += lVals[i];
	}

	// Now roll the dice to see what value to pick
	iTotalWeight = Calc_GetRandVal(iTotalWeight) + 1;
	// Now loop to that value
	i = 1;
	for ( ; i<iQty; i+=2 )
	{
		iTotalWeight -= lVals[i];
		if ( iTotalWeight <= 0 )
			break;
	}

	return( lVals[i-1] );
}
