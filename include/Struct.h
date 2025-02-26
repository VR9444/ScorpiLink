#ifndef STRUCT_H
#define STRUCT_H

namespace SysData
{

    struct SysData
    {
        int droneReleaseStatus{2}; // 2 - Not Ready
                                   // 1 - Ready
                                   // 0 - Deployed

        // Constructor initializes the variable (default: false)
        SysData() : droneReleaseStatus(2) {}
    };

} // namespace SysData

#endif // STRUCT_H
