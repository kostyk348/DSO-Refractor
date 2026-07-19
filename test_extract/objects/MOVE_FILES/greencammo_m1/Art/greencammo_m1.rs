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
			alphaTest greater 0.8;

			stage
			{
				texture "texture/Greencamo";
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
			lighting true;
			lightingSpecular true;
			materialAmbient 1 1 1;
			materialDiffuse 1 1 1;
			materialSpecular 0 0 0;
			materialSpecularPower 12.5;

			stage
			{
				texture "texture/camopole";
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

			stage
			{
				texture "texture/camopole";
				combine color mul texture diffuse;
				combine alpha single texture;
			}
		}
	}
}

