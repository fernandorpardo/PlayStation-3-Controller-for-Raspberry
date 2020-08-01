#ifndef __JOYSTICK_EFFECTS__
#define __JOYSTICK_EFFECTS__

#define N_EFFECTS 2
#define DEV_EVENT	"/dev/input/event0"

// types defines in <linux/input.h>
// https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h
class JoystickEffects
{
	public:
		JoystickEffects(void);
		~JoystickEffects(void);
		void create(__u16 strong, __u16 weak, __u16 length, __u16 delay);
		void play(int i);
	private:
		int fd;
		__s16 id[N_EFFECTS];
		int n; 
};

JoystickEffects::JoystickEffects(void)
{
	n=0;
	memset(&id, 0, sizeof(id));
	fd = open(DEV_EVENT, O_RDWR);
	if (fd == -1) {
		perror("[JoystickEffects] Open device file");
	}
	fprintf(stdout, "\nDevice %s opened", DEV_EVENT);
	fflush(stdout);
}
void JoystickEffects::create(__u16 strong, __u16 weak, __u16 length, __u16 delay)
{
	struct ff_effect effects;
	if(n>=N_EFFECTS) return;
	//rumbling effect 
	effects.type = FF_RUMBLE;
	effects.id = -1;
	effects.u.rumble.strong_magnitude = strong;
	effects.u.rumble.weak_magnitude = weak;
	effects.replay.length = length;
	effects.replay.delay = delay;
	fprintf(stdout,"\nUploading effect ... ");
	fflush(stdout);
	if (ioctl(fd, EVIOCSFF, &effects) == -1) {
		perror("[JoystickEffects::create]");
		return;
	}
	id[n++]= effects.id;
}
void JoystickEffects::play(int i)
{
	struct input_event play;
	if(i>=n) return;
	memset(&play, 0, sizeof(play));
	play.type = EV_FF;
	play.code = id[i];
	play.value = 1;
	if (write(fd, (const void*) &play, sizeof(play)) == -1) {
		perror("[JoystickEffects::play]");
		return;
	}
	fprintf(stdout, "\nPlaying: id=%d", id[i]); 
}
JoystickEffects::~JoystickEffects(void) 
{
	fprintf(stdout, "\nStopping effects");
	fflush(stdout);
	for (int i=0; i<n; ++i) {
		struct input_event stop;
		memset(&stop, 0, sizeof(stop));
		stop.type = EV_FF;
		stop.code =  id[i];
		stop.value = 0;
		if (write(fd, (const void*) &stop, sizeof(stop)) == -1) {
			char str[128];
			snprintf(str, sizeof(str),"[JoystickEffects] ERROR stopping %d/%d", i, id[i]);
			perror(str);
		}
	}	
	fprintf(stdout, "\n[JoystickEffects] Close %s\n", DEV_EVENT);
	fflush(stdout);
	if(fd!=-1) close(fd);
}
#endif
