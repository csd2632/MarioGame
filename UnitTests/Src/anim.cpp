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
	AnimatorManager::GetSingleton().MarkAsSuspended(this);
	if (onFinish) {
		(onFinish)(this);
	}
}
void Animator::NotifyAction (const Animation& anim) {
	if (onAction)
		(onAction)(this, anim);
}
void Animator::TimeShift (timestamp_t offset) {
	lastTime += offset;
}

void MovingAnimator::Progress (timestamp_t currTime) {
	while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
		lastTime += anim->GetDelay();
		NotifyAction(*anim);
		if (!anim->IsForever() && ++currRep == anim->GetReps()) {
			state = ANIMATOR_FINISHED;
			NotifyStopped();
		}
	}
}

void Sprite_MoveAction (Sprite* sprite, const  MovingAnimation& anim) {
	sprite->Move(anim.GetDx(), anim.GetDy());
}

/*animator->SetOnAction(
	[sprite](Animator* animator, const Animation& anim) {
		Sprite_MoveAction(sprite, (const MovingAnimation&) anim);
	}
);*/

void FrameRangeAnimator::Progress (timestamp_t currTime) {
	while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
		if (currFrame == anim->GetEndFrame()) {
			assert(anim->IsForever() || currRep < anim->GetReps());
			currFrame = anim->GetStartFrame();
		}
		else {
			++currFrame;
		}
		lastTime += anim->GetDelay();
		NotifyAction (*anim);
		if (currFrame == anim->GetEndFrame()) {
			if (!anim->IsForever() && ++currRep == anim->GetReps()) {
				state = ANIMATOR_FINISHED;
				NotifyStopped ();
				return;
			}
		}
	}
}

void FrameRange_Action (Sprite* sprite, Animator* animator, const FrameRangeAnimation& anim) {
	auto* frameRangeAnimator = (FrameRangeAnimator*) animator;
	if (frameRangeAnimator->GetCurrFrame() != anim.GetStartFrame() ||
		frameRangeAnimator->GetCurrRep()) {
			sprite->Move(anim.GetDx(), anim.GetDy());
			sprite->SetFrame(frameRangeAnimator->GetCurrFrame());
		}
}

/*animator->SetOnAction(
	[sprite](Animator* animator, const Animation& anim) {
		FrameRange_Action(sprite, animator, (const FrameRangeAnimation&) anim);
	}
);*/


void Animator::NotifyStarted (void) {
	AnimatorManager::GetSingleton().MarkAsRunning(this);
	if (onStart) {
		(onStart)(this);
	}
}

Animator::Animator (void) {
	AnimatorManager::GetSingleton().Register(this);
}

Animator::~Animator (void) {
	AnimatorManager::GetSingleton().Cancel(this);
}

void LatelyDestroyable::Delete (void) {
	assert(!dying); dying = true; delete this;
}

void DestructionManager::Register (LatelyDestroyable* d) {
	assert(!d->IsAlive());
	dead.push_back(d);
}

uint64_t SystemClock::milli_secs (void) const {
	return std::chrono::duration_cast<std::chrono::milliseconds> (clock.now().time_since_epoch()).count();
}
uint64_t SystemClock::micro_secs (void) const {
	return std::chrono::duration_cast<std::chrono::microseconds>(clock.now().time_since_epoch()).count();
}
uint64_t SystemClock::nano_secs (void) const {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(clock.now().time_since_epoch()).count();
}
uint64_t GetSystemTime (void) {
	return SystemClock::Get().milli_secs();
}

void TickAnimator::Progress (timestamp_t currTime) {
	if (!anim->IsDiscrete()) {
		elapsedTime = currTime - lastTime;
		lastTime = currTime;
		NotifyAction(*anim);
	}
	else
	while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay())   {
		lastTime += anim->GetDelay();
		NotifyAction(*anim);
		if (!anim->IsForever() && ++currRep == anim->GetReps()) {
			state = ANIMATOR_RUNNING;
			NotifyStopped();
			return;
		}
	}
}

void MotionQuantizer::Move (const Rect& r, int *dx, int *dy) {
	if (!used) {
		mover(r, dx, dy);
	}
	else {
		do {
			auto dxFinal = std::min(number_sign(*dx)*horizMax, *dx);
			auto dyFinal = std::min(number_sign(*dy)*vertMax, *dy);
			mover(r, &dxFinal, &dyFinal);
			if (!dxFinal) 	{*dx = 0;}
			else 				{*dx -= dxFinal;}
			if (!dyFinal)	{*dy = 0;}
			else 				{*dy -= dyFinal;}
		} while (*dx || *dy);
	}
}

template <class T> bool clip_rect (
	T x, 		T y, 		T w, 		T h,
	T wx,		T wy,		T ww,		T wh,
	T* cx,	T* cy,	T* cw,	T* ch
) {
	*cw = T(std::min(wx + ww, x + w)) - (*cx = T(std::max(wx, x)));
	*ch = T(std::min(wy + wh, y + h)) - (*cy = T(std::max(wy, y)));
	return *cw > 0 && *ch > 0;
}

bool clip_rect (const Rect& r, const Rect& area, Rect* result) {
	return clip_rect (
		r.x,
		r.y,
		r.w,
		r.h,
		area.x,
		area.y,
		area.w,
		area.h,
		&result->x,
		&result->y,
		&result->w,
		&result->h
	);
}

bool Clipper::Clip (const Rect& r, const Rect& dpyArea, Point* dpyPos, Rect* clippedBox) const {
	Rect visibleArea;
	if (!clip_rect(r, *view(), &visibleArea)) 
		{ clippedBox->w = clippedBox->h = 0; return false; }
	else {
		clippedBox->x = r.x - visibleArea.x;
		clippedBox->y = r.y - visibleArea.y;

		clippedBox->w = visibleArea.w;
		clippedBox->h = visibleArea.h;

		dpyPos->x = dpyArea.x + (visibleArea.x - view()->x);
		dpyPos->y = dpyArea.y + (visibleArea.y - view()->y);

		return true;
	}
}

const Clipper MakeTileLayerClipper (TileLayer* layer) {
	return Clipper().SetView(
		[layer](void)
			{ return layer->GetViewWindow();}
	);
}

const Sprite::Mover MakeSpriteGridLayerMover (GridLayer* gridLayer, Sprite* sprite) {
	return [gridLayer, sprite](const Rect& r, int* dx, int* dy) {
		gridLayer->FilterGridMotion(sprite->GetBox(), dx, dy);
	};
}
void Sprite::Display (Bitmap dest, const Rect& dpyArea, const Clipper& clipper) const {
	Rect	clippedBox;
	Point dpyPos;
	if( clipper.Clip(GetBox(), dpyArea, &dpyPos, &clippedBox)) {
		Rect clippedFrame{
			frameBox.x + clippedBox.x,
			frameBox.y + clippedBox.y,
			clippedBox.w,
			clippedBox.h
		};
		MaskedBlit(
			currFilm->GetBitmap(),
			clippedFrame,
			dest,
			dpyPos
		);
	}
}

void GravityHandler::Check (const Rect& r) {
	if (gravityAddicted) {
		if (onSolidGround(r)) {
			if (isFalling) {
				isFalling = false;
				OnStopFalling();
			}
		}
		else if (!isFalling) {
			isFalling = true;
			onStartFalling();
		}
	}
}

void CollisionChecker::Cancel (Sprite* s1, Sprite* s2) {
	auto i = std::find_if(
		entries.begin(),
		entries.end(),
		[s1, s2](const Entry& e) {
			return 	std::get<0>(e) 	== s1 && std::get<1>(e) == s2 ||
						std::get<0>(e) 	== s2 && std::get<1>(e) == s1;
		}
	);
}

int AnimationFilmHolder::ParseEntry (
int startPos,
std::ifstream& f,
std::string& id,
std::string& path,
std::vector<Rect>& rects) {
	int x, y, w, h;
	if (f >> id >> path >> x >> y >> w >> h) {
		rects.push_back({x, y, w, h});
		return 1;
	}
	return 0;
}