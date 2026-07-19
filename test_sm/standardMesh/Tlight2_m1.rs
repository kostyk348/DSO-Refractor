subshader "Tlight2_m1_Material1" "StandardMesh/Default"
{
	lighting false;
	transparent true;
	blendSrc sourceAlpha; 
	blendDest one;
	depthWrite false;
	alphaTestRef 0.7;
	texture "texture/tracklight_s";
}

