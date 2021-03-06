#pragma once

#include <vpp/core/image2d.hh>
#include <vpp/draw/draw.hh>
#include <vpp/core/keypoint_trajectory.hh>
#include <iod/sio_utils.hh>
#include <miscellanous/operations.hh>


namespace vppx
{
using namespace vpp;

template <typename... OPTS>
void draw_trajectories_hough(image2d<vuchar3>& out, std::vector<keypoint_trajectory>& trs,
                             int max_trajectory_len,int T_theta,int nrows,int ncols,
                             OPTS... opts)
{
    auto options = iod::D(opts...);

    auto trajectory_color = options.get(_trajectory_color, [] (int i) {
        return vuchar3(255, 255, 0); });

    if (max_trajectory_len == -1)
        max_trajectory_len = INT_MAX;

    std::vector<int> sorted_idx;
    for (auto& t : trs) sorted_idx.push_back(sorted_idx.size());

    std::sort(sorted_idx.begin(), sorted_idx.end(), [&] (auto i, auto j) { return trs[i].start_frame() > trs[j].start_frame(); });

#pragma omp parallel for
    for (int ti = 0; ti < trs.size(); ti++)
    {
        keypoint_trajectory& t = trs[sorted_idx[ti]];
        if (!t.alive() or t.size() == 0) continue;

        vint2 p1 = t.position_at_frame(t.end_frame()).template cast<int>();
        vint2 p2 = t.position_at_frame(t.end_frame() - std::min(10, int(t.size()) - 1)).template cast<int>();

        vuchar3 pt_color = hsv_to_rgb((M_PI + atan2(p2[0] - p1[0], p2[1] - p1[1])) * 180.f / M_PI, 1.f, 1.f);

        int last_frame_id = std::max(t.end_frame() - max_trajectory_len, t.start_frame());
        if ((p1 - t.position_at_frame(last_frame_id).template cast<int>()).norm() < 4)
            continue;

        for (int i = t.end_frame(); i >= std::max(t.end_frame() - max_trajectory_len, t.start_frame()) + 1; i--)
        {
            vint2 p1 = t.position_at_frame(i).template cast<int>();
            vint2 p2 = t.position_at_frame(i - 1).template cast<int>();

            vint4 ligne1 = getLineFromPoint(p1[0],p1[1],T_theta,nrows,ncols);
            vint4 ligne2 = getLineFromPoint(p2[0],p2[1],T_theta,nrows,ncols);

            //vector<cv::Point> points(4);
            //points = { (x1,y1),(x2,y2),(x3,y3),(x4,y4) };

            //std::cout << " de : (" << p1[0] << "," << p1[1] << ")  à  (" << p2[0]  << "," << p2[1] << ")" << std::endl;
            vuchar4 color;
            color.segment<3>(0) = pt_color;
            color[3] = 0.4f*(255.f - 255.f * (t.end_frame() - i) / max_trajectory_len);

            draw::line2d(out, vint2(ligne1[1],ligne1[0]), vint2(ligne1[3],ligne1[2]),
                         color
                         );
            draw::line2d(out, vint2(ligne2[1],ligne2[0]), vint2(ligne2[3],ligne2[2]),
                         color
                         );
        }

        //draw::c9(out, t.position().template cast<int>(), pt_color);
        draw::c9(out, t.position().template cast<int>(), vuchar3(0,0,255));

    }
}

}
