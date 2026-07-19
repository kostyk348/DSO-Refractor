subshader "Richo_sandcorn_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	transparent true;
	blendSrc sourceAlpha; 
	blendDest one;
	twosided true;
	depthWrite false;
	alphaTestRef 0.7;
	texture "texture/e_PouringSand";
}

