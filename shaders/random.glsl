// taken from https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83

struct RandomState {
    float seed;
};

float rand(out RandomState state){
    float val = fract(sin(state.seed) * 43758.5453123);
    state.seed = val;
    return val;
}

// float noise(float p){
// 	float fl = floor(p);
//     float fc = fract(p);
// 	return mix(rand(fl), rand(fl + 1.0), fc);
// }
	
// float noise(vec2 n) {
// 	const vec2 d = vec2(0.0, 1.0);
//     vec2 b = floor(n), f = smoothstep(vec2(0.0), vec2(1.0), fract(n));
// 	return mix(mix(rand(b), rand(b + d.yx), f.x), mix(rand(b + d.xy), rand(b + d.yy), f.x), f.y);
// }