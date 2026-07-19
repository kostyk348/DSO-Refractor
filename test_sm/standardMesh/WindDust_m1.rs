subshader "WindDust_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 0.145098 0.145098 0.145098;
	blendSrc sourceAlpha; 
	blendDest invsourceAlpha;
	transparent true;
	depthWrite false;
	alphaTestRef 0.5;
	texture "texture/E_dustwind_I";
}

