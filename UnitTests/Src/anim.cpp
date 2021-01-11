#include "anim.hpp"

void Animator::Finish (bool isForced) {
	if (!HasFinished()) {
		state = isForced ? ANIMATOR_STOPPED : ANIMATOR_FINISHED;
		NotifyStopped();
	}
}
void Animator::Stop (void) {
	Finish(true);
}
void Animator::NotifyStopped (void) {
	if (onFinish) 
		(onFinish)(this);
}
void Animator::NotifyAction (const Animation& anim) {
	if (onAction)
		(onAction)(this, anim);
}
void Animator::TimeShift (timestamp_t offset) {
	lastTime +- offset;
}