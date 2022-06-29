/*
Author
~~~~~~
Twitter: @javidx9
Blog: http://www.onelonecoder.com
Discord: https://discord.gg/WhwHUMV

Video:
~~~~~~
https://youtu.be/FlieT66N9OM
*/

#include <iostream>
#include <string>
using namespace std;

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

struct sPoint2D
{
	float x;
	float y;
	float length;
};

struct sSpline
{
	vector<sPoint2D> points;
	float fTotalSplineLength = 0.0f;

	sPoint2D GetSplinePoint(float t)
	{
		int p0, p1, p2, p3;

		p1 = ((int)t) % points.size();
		p2 = (p1 + 1) % points.size();
		p3 = (p2 + 1) % points.size();
		p0 = p1 >= 1 ? p1 - 1 : points.size() - 1;
	
		t = t - (int)t;

		float tt = t * t;
		float ttt = tt * t;

		float q1 = -ttt + 2.0f * tt - t;
		float q2 = 3.0f * ttt - 5.0f * tt + 2.0f;
		float q3 = -3.0f * ttt + 4.0f * tt + t;
		float q4 = ttt - tt;

		float tx = 0.5f * (points[p0].x * q1 + points[p1].x * q2 + points[p2].x * q3 + points[p3].x * q4);
		float ty = 0.5f * (points[p0].y * q1 + points[p1].y * q2 + points[p2].y * q3 + points[p3].y * q4);

		return{ tx, ty };
	}

	sPoint2D GetSplineGradient(float t)
	{
		int p0, p1, p2, p3;

		p1 = ((int)t) % points.size();
		p2 = (p1 + 1) % points.size();
		p3 = (p2 + 1) % points.size();
		p0 = p1 >= 1 ? p1 - 1 : points.size() - 1;

		t = t - (int)t;

		float tt = t * t;
		float ttt = tt * t;

		float q1 = -3.0f * tt + 4.0f * t - 1.0f;
		float q2 = 9.0f * tt - 10.0f * t;
		float q3 = -9.0f * tt + 8.0f * t + 1.0f;
		float q4 = 3.0f * tt - 2.0f * t;

		float tx = 0.5f * (points[p0].x * q1 + points[p1].x * q2 + points[p2].x * q3 + points[p3].x * q4);
		float ty = 0.5f * (points[p0].y * q1 + points[p1].y * q2 + points[p2].y * q3 + points[p3].y * q4);

		return{ tx, ty };
	}

	float CalculateSegmentLength(int node)
	{
		float fLength = 0.0f;
		float fStepSize = 0.1;

		sPoint2D old_point, new_point;
		old_point = GetSplinePoint((float)node);

		for (float t = 0; t < 1.0f; t += fStepSize)
		{
			new_point = GetSplinePoint((float)node + t);
			fLength += sqrtf((new_point.x - old_point.x) * (new_point.x - old_point.x)
				+ (new_point.y - old_point.y) * (new_point.y - old_point.y));
			old_point = new_point;
		}

		return fLength;
	}


	float GetNormalisedOffset(float p)
	{
		// Which node is the base?
		int i = 0;
		while (p > points[i].length)
		{
			p -= points[i].length;
			i++;
		}

		// The fractional is the offset 
		return (float)i + (p / points[i].length);
	}

	void UpdateSplineProperties()
	{
		// Use to cache local spline lengths and overall spline length
		fTotalSplineLength = 0.0f;

		// Each node has a succeeding length
		for (int i = 0; i < points.size(); i++)
		{
			points[i].length = CalculateSegmentLength(i);
			fTotalSplineLength += points[i].length;
		}
	}

	void DrawSelf(olc::PixelGameEngine* gfx, olc::Pixel p = olc::WHITE)
	{
		for (float t = 0; t < (float)points.size(); t += 1)
		{
			sPoint2D pos = GetSplinePoint(t);
			gfx->Draw(pos.x, pos.y, p);
		}
	}

	void DrawPathSelf(olc::PixelGameEngine* gfx, float distance, sSpline &pathFill, olc::Pixel p = olc::WHITE)
	{
		int k = 0;
		for (float t = 0; t < (float)points.size(); t += distance)
		{
			sPoint2D pos = GetSplinePoint(t);
			pathFill.points[k] = pos;
			k++;
			gfx->Draw(pos.x, pos.y, p);
		}
	}

	void DrawBoundariesSelf(olc::PixelGameEngine* gfx, sSpline pathFill, float fTrackWidth, olc::Pixel p)
	{
		for (float t = 0; t < (float)points.size(); t += 1.0)
		{
			sPoint2D p1 = pathFill.GetSplinePoint(t);
			sPoint2D g1 = pathFill.GetSplineGradient(t);
			float glen = sqrtf(g1.x * g1.x + g1.y * g1.y);

			if (p == olc::YELLOW)
			{
				points[t].x = p1.x + fTrackWidth * (-g1.y / glen);
				points[t].y = p1.y + fTrackWidth * (g1.x / glen);
			}
			else
			{
				points[t].x = p1.x - fTrackWidth * (-g1.y / glen);
				points[t].y = p1.y - fTrackWidth * (g1.x / glen);
			}

			gfx->Draw(points[t].x, points[t].y, p);
		}
	}
};

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

class RacingTrack : public olc::PixelGameEngine
{
public:
	RacingTrack()
	{
		sAppName = "Racing Track";
	}

private:
	sSpline path, pathFill, trackInside, trackOutside;	// Various splines

	int nNodes = 15;	       // Number of red (controlable) nodes in spline
	int nNodesSpline = 150;    // Number of total points in a spline
	int nSelectedNode = -1;
	float fTrackWidth = 10.0f; // Width of the track

	void exportToCSV()
	{
		ofstream fout("data/data.csv");

		fout << "xPath" << "," << "yPath" << "," << "xOutside" << "," << "yOutside" << "," << "xInside" << "," << "yInside" << endl;
		for (int i = 0; i < pathFill.points.size(); i += 1)
		{
			fout << pathFill.points[i].x << "," << pathFill.points[i].y << "," << trackOutside.points[i].x << "," << trackOutside.points[i].y
				<< "," << trackInside.points[i].x << "," << trackInside.points[i].y << endl;
		}

		fout.close();
	}

protected:
	// Called by olcPixelGameEngine
	virtual bool OnUserCreate()
	{
		for (int i = 0; i < nNodes; i++)
		{
			path.points.push_back(
				{ 30.0f * sinf((float)i / (float)nNodes * 3.14159f * 2.0f) + ScreenWidth() / 2,
				30.0f * cosf((float)i / (float)nNodes * 3.14159f * 2.0f) + ScreenHeight() / 2 });
		}

		for (int i = 0; i < nNodesSpline; i++)
		{
			pathFill.points.push_back({ 0.0f, 0.0f });
			trackInside.points.push_back({ 0.0f, 0.0f });
		    trackOutside.points.push_back({ 0.0f, 0.0f });
		}

		path.UpdateSplineProperties();
		return true;
	}

	// Called by olcPixelGameEngine
	virtual bool OnUserUpdate(float fElapsedTime)
	{
		// Clear Screen
		FillRect(0, 0, ScreenWidth(), ScreenHeight(), olc::BLACK);

		FillRect(240, 220, 10, 10, olc::RED);

		// Check if exit button is pressed and export the data to a CSV file
		if (GetMouse(0).bHeld)
		{
			for (int i = 220; i < 230; i++)
			{
				for (int p = 240; p < 250; p++)
				{
					if (GetMouseX() == p && GetMouseY() == i)
					{
						FillRect(240, 220, 10, 10, olc::DARK_RED);

						exportToCSV();

						olc_Terminate();
					}
				}
			}
		}

		// Check if node is selected with mouse
		if (GetMouse(0).bPressed)
		{
			for (int i = 0; i < path.points.size(); i++)
			{
				float d = sqrtf(powf(path.points[i].x - GetMouseX(), 2) + powf(path.points[i].y - GetMouseY(), 2));
				if (d < 5.0f)
				{
					nSelectedNode = i;
					break;
				}
			}
		}

		if (GetMouse(0).bReleased)
			nSelectedNode = -1;

		// Move selected node
		if (GetMouse(0).bHeld && nSelectedNode >= 0)
		{
			path.points[nSelectedNode].x = GetMouseX();
			path.points[nSelectedNode].y = GetMouseY();
			path.UpdateSplineProperties();
		}

		// Draw Track
		path.DrawSelf(this);
		path.DrawPathSelf(this, (float)nNodes / (float)nNodesSpline, pathFill);
		trackInside.DrawBoundariesSelf(this, pathFill, fTrackWidth, olc::BLUE);
		trackOutside.DrawBoundariesSelf(this, pathFill, fTrackWidth, olc::YELLOW);

		for (auto i : path.points)
			FillPixels(i.x - 1, i.y - 1, i.x + 2, i.y + 2, olc::RED);

		return true;
	}
};

int main()
{
	RacingTrack demo;
	demo.Construct(256, 240, 4, 4);
	demo.Start();

	return 0;
}