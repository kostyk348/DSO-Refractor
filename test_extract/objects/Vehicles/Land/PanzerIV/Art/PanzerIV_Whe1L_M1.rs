shader "Material0"
{
	technique
	{
		pass
		{
			cullMode none;
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 12.5;
			alphaTest greater 0.7;

			stage
			{
				texture "texture/P4Whe4_f";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

shader "Material1"
{
	technique
	{
		pass
		{
			cullMode none;
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 12.5;
			alphaTest greater 0.7;


			stage
			{
				texture "texture/P4Whe4_f";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

shader "Material2"
{
	technique
	{
		pass
		{
			cullMode none;
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 12.5;
			alphaTest greater 0.7;


			stage
			{
				texture "texture/p4Whe1_f";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

shader "Material3"
{
	technique
	{
		pass
		{
			cullMode none;
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 12.5;
			alphaTest greater 0.7;


			stage
			{
				texture "texture/p4Whe1_f";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

shader "Material4"
{
	technique
	{
		pass
		{
			cullMode none;
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 12.5;
			alphaTest greater 0.7;


			stage
			{
				texture "texture/P4Whe2_f";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

