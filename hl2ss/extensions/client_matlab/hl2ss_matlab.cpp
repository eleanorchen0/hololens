
#include "mex.hpp"
#include "mexAdapter.hpp"
#include "hl2ss_ulm.h"

namespace hl2ss
{
namespace matlab
{
namespace grab_mode
{
uint8_t const BY_FRAME_INDEX = 0;
uint8_t const BY_TIMESTAMP   = 1;
}
}
}

//------------------------------------------------------------------------------
// (*) MexFunction
//------------------------------------------------------------------------------

class MexFunction : public matlab::mex::Function
{
private:
    std::shared_ptr<matlab::engine::MATLABEngine> m_matlabPtr = getEngine();
    matlab::data::ArrayFactory m_factory;
    uint32_t m_argument_index;
    bool m_initialized;

    std::unique_ptr<hl2ss::shared::source> source_rm_vlc[4];
    std::unique_ptr<hl2ss::shared::source> source_rm_depth_ahat;
    std::unique_ptr<hl2ss::shared::source> source_rm_depth_longthrow;
    std::unique_ptr<hl2ss::shared::source> source_rm_imu[3];
    std::unique_ptr<hl2ss::shared::source> source_pv;
    std::unique_ptr<hl2ss::shared::source> source_microphone;
    std::unique_ptr<hl2ss::shared::source> source_si;
    std::unique_ptr<hl2ss::shared::source> source_eet;
    std::unique_ptr<hl2ss::shared::source> source_extended_audio;
    std::unique_ptr<hl2ss::shared::source> source_ev;
    std::unique_ptr<hl2ss::shared::source> source_extended_depth;

    std::unique_ptr<hl2ss::shared::ipc_rc>  ipc_rc;
    std::unique_ptr<hl2ss::shared::ipc_sm>  ipc_sm;
    std::unique_ptr<hl2ss::shared::ipc_su>  ipc_su;
    std::unique_ptr<hl2ss::shared::ipc_vi>  ipc_vi;
    std::unique_ptr<hl2ss::shared::ipc_umq> ipc_umq;
    std::unique_ptr<hl2ss::shared::ipc_gmq> ipc_gmq;

    hl2ss::ulm::configuration_microphone     configuration_microphone;
    hl2ss::ulm::configuration_extended_audio configuration_extended_audio;

public:
    //------------------------------------------------------------------------------
    // (*) Helpers
    //------------------------------------------------------------------------------

    static void default_deleter(void* p)
    {
        delete[] p;
    }

    template <typename T>
    matlab::data::TypedArray<T> get_argument_array(matlab::mex::ArgumentList inputs)
    {
        if (m_argument_index >= inputs.size()) { throw std::runtime_error("Not enough inputs"); }
        return inputs[m_argument_index++];
    }

    template <typename T>
    T get_argument(matlab::mex::ArgumentList inputs)
    {
        return get_argument_array<T>(inputs)[0];
    }

    std::string get_argument_string(matlab::mex::ArgumentList inputs)
    {
        matlab::data::CharArray argument = get_argument_array<CHAR16_T>(inputs);
        return argument.toAscii();
    }

    template <typename T>
    std::vector<T> to_std_vector(matlab::data::TypedArray<T> t)
    {
        return std::vector<T>(t.begin(), t.end());
    }

    template <typename T>
    T* get_pointer(matlab::data::TypedArray<T>& array)
    {
        return array.begin().operator->();
    }

    template <typename T, size_t size>
    matlab::data::TypedArray<T> to_typed_array(T const (&array)[size], matlab::data::ArrayDimensions dims)
    {
        return m_factory.createArray<T>(dims, &array[0], &array[size]);
    }

    template <typename T>
    matlab::data::TypedArray<T> to_typed_array(void const* data, size_t size, matlab::data::ArrayDimensions dims)
    {
        return m_factory.createArray<T>(dims, (T*)data, (T*)(((uint8_t*)data) + size));
    }

    template <typename T>
    T get_field_scalar(matlab::data::TypedArray<T> array)
    {
        return array[0];
    }

    hl2ss::vector_2 get_field_vector_2(matlab::data::TypedArray<float> array)
    {
        return { array[0], array[1] };
    }

    hl2ss::vector_3 get_field_vector_3(matlab::data::TypedArray<float> array)
    {
        return { array[0], array[1], array[2] };
    }

    hl2ss::vector_4 get_field_vector_4(matlab::data::TypedArray<float> array)
    {
        return { array[0], array[1], array[2], array[3] };
    }

    hl2ss::guid get_field_guid(matlab::data::TypedArray<uint64_t> array)
    {
        return { array[0], array[1] };
    }

    void error(char const* message)
    {
        m_matlabPtr->feval(u"error", 0, std::vector<matlab::data::Array>{ m_factory.createScalar(message) });
    }

    //------------------------------------------------------------------------------
    // (*) Open
    //------------------------------------------------------------------------------

    void open_rm_vlc(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_rm_vlc();

        configuration.chunk   = get_argument<uint64_t>(inputs);
        configuration.mode    = get_argument<uint8_t>(inputs);
        configuration.divisor = get_argument<uint8_t>(inputs);
        configuration.profile = get_argument<uint8_t>(inputs);
        configuration.level   = get_argument<uint8_t>(inputs);
        configuration.bitrate = get_argument<uint32_t>(inputs);

        auto options = get_argument_array<uint64_t>(inputs);
        
        configuration.options_data = get_pointer(options);
        configuration.options_size = options.getNumberOfElements();

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);

        source_rm_vlc[port - hl2ss::stream_port::RM_VLC_LEFTFRONT] = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
    }

    void open_rm_depth_ahat(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_rm_depth_ahat();

        configuration.chunk      = get_argument<uint64_t>(inputs);
        configuration.mode       = get_argument<uint8_t>(inputs);
        configuration.divisor    = get_argument<uint8_t>(inputs);
        configuration.profile_z  = get_argument<uint8_t>(inputs);
        configuration.profile_ab = get_argument<uint8_t>(inputs);
        configuration.level      = get_argument<uint8_t>(inputs);
        configuration.bitrate    = get_argument<uint32_t>(inputs);

        auto options = get_argument_array<uint64_t>(inputs);

        configuration.options_data = get_pointer(options);
        configuration.options_size = options.getNumberOfElements();

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);

        source_rm_depth_ahat = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
    }

    void open_rm_depth_longthrow(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_rm_depth_longthrow();

        configuration.chunk      = get_argument<uint64_t>(inputs);
        configuration.mode       = get_argument<uint8_t>(inputs);
        configuration.divisor    = get_argument<uint8_t>(inputs);
        configuration.png_filter = get_argument<uint8_t>(inputs);

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);

        source_rm_depth_longthrow = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
    }

    void open_rm_imu(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_rm_imu();

        configuration.chunk = get_argument<uint64_t>(inputs);
        configuration.mode  = get_argument<uint8_t>(inputs);

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);
        
        source_rm_imu[port - hl2ss::stream_port::RM_IMU_ACCELEROMETER] = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
    }
 
    void open_pv(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_pv();

        configuration.width     = get_argument<uint16_t>(inputs);
        configuration.height    = get_argument<uint16_t>(inputs);
        configuration.framerate = get_argument<uint8_t>(inputs);
        configuration.chunk     = get_argument<uint64_t>(inputs);
        configuration.mode      = get_argument<uint8_t>(inputs);
        configuration.divisor   = get_argument<uint8_t>(inputs);
        configuration.profile   = get_argument<uint8_t>(inputs);
        configuration.level     = get_argument<uint8_t>(inputs);
        configuration.bitrate   = get_argument<uint32_t>(inputs);

        auto options = get_argument_array<uint64_t>(inputs);

        configuration.options_data = get_pointer(options);
        configuration.options_size = options.getNumberOfElements();

        uint8_t  decoded_format = get_argument<uint8_t>(inputs) % 5;
        uint64_t buffer_size    = get_argument<uint64_t>(inputs);

        switch (port)
        {
        case hl2ss::stream_port::PERSONAL_VIDEO: source_pv = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded_format); break;
        case hl2ss::stream_port::EXTENDED_VIDEO: source_ev = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded_format); break;
        }
    }

    void open_microphone(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_microphone();

        configuration.chunk   = get_argument<uint64_t>(inputs);
        configuration.profile = get_argument<uint8_t>(inputs);
        configuration.level   = get_argument<uint8_t>(inputs);

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);
        
        source_microphone = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
        
        configuration_microphone = configuration;
    }

    void open_si(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_si();

        configuration.chunk = get_argument<uint64_t>(inputs);

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);

        source_si = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
    }

    void open_eet(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_eet();

        configuration.chunk = get_argument<uint64_t>(inputs);
        configuration.fps   = get_argument<uint8_t>(inputs);

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);

        source_eet = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
    }

    void open_extended_audio(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_extended_audio();

        configuration.chunk           = get_argument<uint64_t>(inputs);
        configuration.mixer_mode      = get_argument<uint32_t>(inputs);
        configuration.loopback_gain   = get_argument<float>(inputs);
        configuration.microphone_gain = get_argument<float>(inputs);
        configuration.profile         = get_argument<uint8_t>(inputs);
        configuration.level           = get_argument<uint8_t>(inputs);

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);

        source_extended_audio = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
    
        configuration_extended_audio = configuration;
    }

    void open_extended_depth(char const* host, uint16_t port, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_extended_depth();

        configuration.chunk       = get_argument<uint64_t>(inputs);
        configuration.media_index = get_argument<uint64_t>(inputs);
        configuration.stride_mask = get_argument<uint64_t>(inputs);
        configuration.mode        = get_argument<uint8_t>(inputs);
        configuration.divisor     = get_argument<uint8_t>(inputs);
        configuration.profile_z   = get_argument<uint8_t>(inputs);

        bool     decoded     = true;
        uint64_t buffer_size = get_argument<uint64_t>(inputs);

        source_extended_depth = hl2ss::svc::open_stream<hl2ss::shared::source>(host, port, buffer_size, &configuration, decoded);
    }

    void open(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::string host = get_argument_string(inputs);
        uint16_t    port = get_argument<uint16_t>(inputs);
        
        switch (port)
        {
        // Stream
        case hl2ss::stream_port::RM_VLC_LEFTFRONT:
        case hl2ss::stream_port::RM_VLC_LEFTLEFT:
        case hl2ss::stream_port::RM_VLC_RIGHTFRONT:
        case hl2ss::stream_port::RM_VLC_RIGHTRIGHT:    open_rm_vlc(            host.c_str(), port, inputs); break;
        case hl2ss::stream_port::RM_DEPTH_AHAT:        open_rm_depth_ahat(     host.c_str(), port, inputs); break;
        case hl2ss::stream_port::RM_DEPTH_LONGTHROW:   open_rm_depth_longthrow(host.c_str(), port, inputs); break;
        case hl2ss::stream_port::RM_IMU_ACCELEROMETER:
        case hl2ss::stream_port::RM_IMU_GYROSCOPE:
        case hl2ss::stream_port::RM_IMU_MAGNETOMETER:  open_rm_imu(            host.c_str(), port, inputs); break;
        case hl2ss::stream_port::PERSONAL_VIDEO:       open_pv(                host.c_str(), port, inputs); break;
        case hl2ss::stream_port::MICROPHONE:           open_microphone(        host.c_str(), port, inputs); break;
        case hl2ss::stream_port::SPATIAL_INPUT:        open_si(                host.c_str(), port, inputs); break;
        case hl2ss::stream_port::EXTENDED_EYE_TRACKER: open_eet(               host.c_str(), port, inputs); break;
        case hl2ss::stream_port::EXTENDED_AUDIO:       open_extended_audio(    host.c_str(), port, inputs); break;
        case hl2ss::stream_port::EXTENDED_VIDEO:       open_pv(                host.c_str(), port, inputs); break;
        case hl2ss::stream_port::EXTENDED_DEPTH:       open_extended_depth(    host.c_str(), port, inputs); break;
        // IPC
        case hl2ss::ipc_port::REMOTE_CONFIGURATION:    ipc_rc  = hl2ss::svc::open_ipc<hl2ss::shared::ipc_rc>( host.c_str(), port); break;
        case hl2ss::ipc_port::SPATIAL_MAPPING:         ipc_sm  = hl2ss::svc::open_ipc<hl2ss::shared::ipc_sm>( host.c_str(), port); break;
        case hl2ss::ipc_port::SCENE_UNDERSTANDING:     ipc_su  = hl2ss::svc::open_ipc<hl2ss::shared::ipc_su>( host.c_str(), port); break;
        case hl2ss::ipc_port::VOICE_INPUT:             ipc_vi  = hl2ss::svc::open_ipc<hl2ss::shared::ipc_vi>( host.c_str(), port); break;
        case hl2ss::ipc_port::UNITY_MESSAGE_QUEUE:     ipc_umq = hl2ss::svc::open_ipc<hl2ss::shared::ipc_umq>(host.c_str(), port); break;
        case hl2ss::ipc_port::GUEST_MESSAGE_QUEUE:     ipc_gmq = hl2ss::svc::open_ipc<hl2ss::shared::ipc_gmq>(host.c_str(), port); break;
        //
        default:                                       throw std::runtime_error("Unknown port");
        }
    }

    //------------------------------------------------------------------------------
    // (*) Close
    //------------------------------------------------------------------------------

    void close(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        uint16_t port = get_argument<uint16_t>(inputs);

        switch (port)
        {
        // Stream
        case hl2ss::stream_port::RM_VLC_LEFTFRONT:     
        case hl2ss::stream_port::RM_VLC_LEFTLEFT:      
        case hl2ss::stream_port::RM_VLC_RIGHTFRONT:    
        case hl2ss::stream_port::RM_VLC_RIGHTRIGHT:    source_rm_vlc[port - hl2ss::stream_port::RM_VLC_LEFTFRONT     ] = nullptr; break;
        case hl2ss::stream_port::RM_DEPTH_AHAT:        source_rm_depth_ahat                                            = nullptr; break;
        case hl2ss::stream_port::RM_DEPTH_LONGTHROW:   source_rm_depth_longthrow                                       = nullptr; break;
        case hl2ss::stream_port::RM_IMU_ACCELEROMETER: 
        case hl2ss::stream_port::RM_IMU_GYROSCOPE:     
        case hl2ss::stream_port::RM_IMU_MAGNETOMETER:  source_rm_imu[port - hl2ss::stream_port::RM_IMU_ACCELEROMETER]  = nullptr; break;
        case hl2ss::stream_port::PERSONAL_VIDEO:       source_pv                                                       = nullptr; break;
        case hl2ss::stream_port::MICROPHONE:           source_microphone                                               = nullptr; break;
        case hl2ss::stream_port::SPATIAL_INPUT:        source_si                                                       = nullptr; break;
        case hl2ss::stream_port::EXTENDED_EYE_TRACKER: source_eet                                                      = nullptr; break;
        case hl2ss::stream_port::EXTENDED_AUDIO:       source_extended_audio                                           = nullptr; break;
        case hl2ss::stream_port::EXTENDED_VIDEO:       source_ev                                                       = nullptr; break;
        case hl2ss::stream_port::EXTENDED_DEPTH:       source_extended_depth                                           = nullptr; break;
        // IPC
        case hl2ss::ipc_port::REMOTE_CONFIGURATION:    ipc_rc  = nullptr; break;
        case hl2ss::ipc_port::SPATIAL_MAPPING:         ipc_sm  = nullptr; break;
        case hl2ss::ipc_port::SCENE_UNDERSTANDING:     ipc_su  = nullptr; break;
        case hl2ss::ipc_port::VOICE_INPUT:             ipc_vi  = nullptr; break;
        case hl2ss::ipc_port::UNITY_MESSAGE_QUEUE:     ipc_umq = nullptr; break;
        case hl2ss::ipc_port::GUEST_MESSAGE_QUEUE:     ipc_gmq = nullptr; break;
        //
        default:                                       throw std::runtime_error("Unknown port");
        }
    }

    //------------------------------------------------------------------------------
    // (*) Grab
    //------------------------------------------------------------------------------

    template <typename T>
    matlab::data::Array unpack_payload(void const* payload, uint32_t offset, uint32_t size, matlab::data::ArrayDimensions dims)
    {
        std::unique_ptr<T[]> payload_copy = std::make_unique<T[]>(size / sizeof(T));
        memcpy(payload_copy.get(), ((uint8_t*)payload) + offset, size);
        return m_factory.createArrayFromBuffer<T>(dims, matlab::data::buffer_ptr_t<T>(payload_copy.release(), default_deleter), matlab::data::MemoryLayout::ROW_MAJOR);
    }

    matlab::data::Array unpack_pose(hl2ss::matrix_4x4 const* pose)
    {
        std::unique_ptr<float[]> pose_copy = std::make_unique<float[]>(sizeof(hl2ss::matrix_4x4) / sizeof(float));
        memcpy(pose_copy.get(), pose, sizeof(hl2ss::matrix_4x4));
        return m_factory.createArrayFromBuffer<float>({4, 4}, matlab::data::buffer_ptr_t<float>(pose_copy.release(), default_deleter), matlab::data::MemoryLayout::ROW_MAJOR);
    }

    void pack_rm_vlc(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, matlab::mex::ArgumentList outputs)
    {
        uint32_t size = hl2ss::parameters_rm_vlc::PIXELS * sizeof(uint8_t);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "image", "sensor_ticks", "exposure", "gain", "pose" });

        o[0]["frame_index"]  = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]       = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]    = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        hl2ss::map_rm_vlc region = hl2ss::unpack_rm_vlc(packet->payload, packet->sz_payload);

        o[0]["image"]        = unpack_payload<uint8_t>(region.image, 0, size, { hl2ss::parameters_rm_vlc::HEIGHT, hl2ss::parameters_rm_vlc::WIDTH });
        o[0]["sensor_ticks"] = m_factory.createScalar<uint64_t>(region.metadata->sensor_ticks);
        o[0]["exposure"]     = m_factory.createScalar<uint64_t>(region.metadata->exposure);
        o[0]["gain"]         = m_factory.createScalar<uint32_t>(region.metadata->gain);
        }
        if (packet->pose)
        {
        o[0]["pose"]         = unpack_pose(packet->pose);
        }
        }

        outputs[0] = std::move(o);
    }

    void pack_rm_depth_ahat(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, matlab::mex::ArgumentList outputs)
    {
        uint32_t size = hl2ss::parameters_rm_depth_ahat::PIXELS * sizeof(uint16_t);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "depth", "ab", "sensor_ticks", "pose" });
        
        o[0]["frame_index"]  = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]       = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]    = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        hl2ss::map_rm_depth_ahat region = hl2ss::unpack_rm_depth_ahat(packet->payload, packet->sz_payload);

        o[0]["depth"]        = unpack_payload<uint16_t>(region.depth, 0, size, { hl2ss::parameters_rm_depth_ahat::HEIGHT, hl2ss::parameters_rm_depth_ahat::WIDTH });
        o[0]["ab"]           = unpack_payload<uint16_t>(region.ab,    0, size, { hl2ss::parameters_rm_depth_ahat::HEIGHT, hl2ss::parameters_rm_depth_ahat::WIDTH });
        o[0]["sensor_ticks"] = m_factory.createScalar<uint64_t>(region.metadata->sensor_ticks);
        }
        if (packet->pose)
        {
        o[0]["pose"]         = unpack_pose(packet->pose);
        }
        }

        outputs[0] = std::move(o);
    }

    void pack_rm_depth_longthrow(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, matlab::mex::ArgumentList outputs)
    {
        uint32_t size = hl2ss::parameters_rm_depth_longthrow::PIXELS * sizeof(uint16_t);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "depth", "ab", "sensor_ticks", "pose" });
        
        o[0]["frame_index"]  = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]       = m_factory.createScalar<int32_t>(status);
        if (packet)
        {
        o[0]["timestamp"]    = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        hl2ss::map_rm_depth_longthrow region = hl2ss::unpack_rm_depth_longthrow(packet->payload, packet->sz_payload);

        o[0]["depth"]        = unpack_payload<uint16_t>(region.depth, 0, size, { hl2ss::parameters_rm_depth_longthrow::HEIGHT, hl2ss::parameters_rm_depth_longthrow::WIDTH });
        o[0]["ab"]           = unpack_payload<uint16_t>(region.ab,    0, size, { hl2ss::parameters_rm_depth_longthrow::HEIGHT, hl2ss::parameters_rm_depth_longthrow::WIDTH });
        o[0]["sensor_ticks"] = m_factory.createScalar<uint64_t>(region.metadata->sensor_ticks);
        }
        if (packet->pose)
        {
        o[0]["pose"]         = unpack_pose(packet->pose);
        }
        }

        outputs[0] = std::move(o);
    }

    void pack_rm_imu(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, matlab::mex::ArgumentList outputs)
    {
        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "sensor_timestamp", "host_timestamp", "x", "y", "z", "temperature", "pose" });

        o[0]["frame_index"]      = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]           = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]        = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        uint32_t samples = packet->sz_payload / sizeof(hl2ss::rm_imu_sample);

        matlab::data::TypedArray<uint64_t> sensor_timestamp = m_factory.createArray<uint64_t>({ samples });
        matlab::data::TypedArray<uint64_t> host_timestamp   = m_factory.createArray<uint64_t>({ samples });
        matlab::data::TypedArray<float>    x                = m_factory.createArray<float>({ samples });
        matlab::data::TypedArray<float>    y                = m_factory.createArray<float>({ samples });
        matlab::data::TypedArray<float>    z                = m_factory.createArray<float>({ samples });
        matlab::data::TypedArray<float>    temperature      = m_factory.createArray<float>({ samples });

        hl2ss::map_rm_imu region = hl2ss::unpack_rm_imu(packet->payload, packet->sz_payload);

        for (uint32_t i = 0; i < samples; ++i)
        {
        sensor_timestamp[i] = region.samples[i].sensor_timestamp;
        host_timestamp[i]   = region.samples[i].timestamp;
        x[i]                = region.samples[i].x;
        y[i]                = region.samples[i].y;
        z[i]                = region.samples[i].z;
        temperature[i]      = region.samples[i].temperature;
        }

        o[0]["sensor_timestamp"] = std::move(sensor_timestamp);
        o[0]["host_timestamp"]   = std::move(host_timestamp);
        o[0]["x"]                = std::move(x);
        o[0]["y"]                = std::move(y);
        o[0]["z"]                = std::move(z);
        o[0]["temperature"]      = std::move(temperature);
        }
        if (packet->pose)
        {
        o[0]["pose"]             = unpack_pose(packet->pose);
        }
        }

        outputs[0] = std::move(o);
    }

    void pack_pv(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, matlab::mex::ArgumentList outputs)
    {
        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "image", "intrinsics", "exposure", "imaging", "gains", "pose" });

        o[0]["frame_index"] = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]      = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]   = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        uint32_t image_size = packet->sz_payload - sizeof(hl2ss::pv_metadata);
        hl2ss::map_pv region = hl2ss::unpack_pv(packet->payload, packet->sz_payload);
        
        uint32_t width    = region.metadata->width;
        uint32_t height   = region.metadata->height;
        uint32_t channels = image_size / (width * height);

        o[0]["image"]       = unpack_payload<uint8_t>(  region.image,                   0, image_size,           { height, width, channels });
        o[0]["intrinsics"]  = unpack_payload<float>(   &region.metadata->f,             0, 4 * sizeof(float),    { 4 });
        o[0]["exposure"]    = unpack_payload<uint64_t>(&region.metadata->exposure_time, 0, 3 * sizeof(uint64_t), { 3 });
        o[0]["imaging"]     = unpack_payload<uint32_t>(&region.metadata->lens_position, 0, 4 * sizeof(uint32_t), { 4 });
        o[0]["gains"]       = unpack_payload<float>   (&region.metadata->iso_gains,     0, 5 * sizeof(float),    { 5 });
        }
        if (packet->pose)
        {
        o[0]["pose"]        = unpack_pose(packet->pose);
        }
        }

        outputs[0] = std::move(o);
    }

    void pack_microphone(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, uint8_t profile, uint8_t level, matlab::mex::ArgumentList outputs)
    {
        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "audio" });

        o[0]["frame_index"] = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]      = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]   = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        if (profile != hl2ss::audio_profile::RAW)
        {
        o[0]["audio"]       = unpack_payload<float>(  packet->payload, 0, packet->sz_payload, { hl2ss::parameters_microphone::CHANNELS,       packet->sz_payload / (sizeof(float)   * hl2ss::parameters_microphone::CHANNELS) });
        }
        else if (level == hl2ss::aac_level::L5)
        {
        o[0]["audio"]       = to_typed_array<float>(  packet->payload,    packet->sz_payload, { hl2ss::parameters_microphone::ARRAY_CHANNELS, packet->sz_payload / (sizeof(float)   * hl2ss::parameters_microphone::ARRAY_CHANNELS) });
        }
        else
        {
        o[0]["audio"]       = to_typed_array<int16_t>(packet->payload,    packet->sz_payload, { hl2ss::parameters_microphone::CHANNELS,       packet->sz_payload / (sizeof(int16_t) * hl2ss::parameters_microphone::CHANNELS) });
        }
        }
        }

        outputs[0] = std::move(o);
    }

    void pack_si(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, matlab::mex::ArgumentList outputs)
    {
        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "valid", "head_pose", "eye_ray", "left_hand", "right_hand" });

        o[0]["frame_index"] = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]      = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]   = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        hl2ss::map_si region = hl2ss::unpack_si(packet->payload, packet->sz_payload);

        o[0]["valid"]       = m_factory.createScalar<uint32_t>({ region.tracking->valid });
        o[0]["head_pose"]   = to_typed_array<float>(&region.tracking->head_pose,  sizeof(region.tracking->head_pose),  { 3, 3 });
        o[0]["eye_ray"]     = to_typed_array<float>(&region.tracking->eye_ray,    sizeof(region.tracking->eye_ray),    { 6 });
        o[0]["left_hand"]   = to_typed_array<float>(&region.tracking->left_hand,  sizeof(region.tracking->left_hand),  { 9, 26 });
        o[0]["right_hand"]  = to_typed_array<float>(&region.tracking->right_hand, sizeof(region.tracking->right_hand), { 9, 26 });
        } 
        }

        outputs[0] = std::move(o);
    }

    void pack_eet(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, matlab::mex::ArgumentList outputs)
    {
        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "combined_ray", "left_ray", "right_ray", "left_openness", "right_openness", "vergence_distance", "valid", "pose" });

        o[0]["frame_index"]       = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]            = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]         = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        hl2ss::map_eet region = hl2ss::unpack_eet(packet->payload, packet->sz_payload);

        o[0]["combined_ray"]      = to_typed_array<float>(&region.tracking->combined_ray, sizeof(region.tracking->combined_ray), { 6 });
        o[0]["left_ray"]          = to_typed_array<float>(&region.tracking->left_ray,     sizeof(region.tracking->left_ray),     { 6 });
        o[0]["right_ray"]         = to_typed_array<float>(&region.tracking->right_ray,    sizeof(region.tracking->right_ray),    { 6 });
        o[0]["left_openness"]     = m_factory.createScalar<float>(region.tracking->left_openness);
        o[0]["right_openness"]    = m_factory.createScalar<float>(region.tracking->right_openness);
        o[0]["vergence_distance"] = m_factory.createScalar<float>(region.tracking->vergence_distance);
        o[0]["valid"]             = m_factory.createScalar<uint32_t>(region.tracking->valid);
        }
        if (packet->pose)
        {
        o[0]["pose"]              = unpack_pose(packet->pose);
        }
        }

        outputs[0] = std::move(o);
    }

    void pack_extended_audio(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, uint8_t profile, uint8_t level, matlab::mex::ArgumentList outputs)
    {
        (void)level;

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "audio" });

        o[0]["frame_index"] = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]      = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]   = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        if (profile != hl2ss::audio_profile::RAW)
        {
        o[0]["audio"]       = unpack_payload<float>(  packet->payload, 0, packet->sz_payload, { hl2ss::parameters_microphone::CHANNELS, packet->sz_payload / (sizeof(float) * hl2ss::parameters_microphone::CHANNELS) });
        }
        else if ((level & 0x80) == 0)
        {
        o[0]["audio"]       = to_typed_array<int16_t>(packet->payload,    packet->sz_payload, { hl2ss::parameters_microphone::CHANNELS, packet->sz_payload / (sizeof(int16_t) * hl2ss::parameters_microphone::CHANNELS) });
        }
        else
        {
        o[0]["audio"]       = to_typed_array<int8_t>( packet->payload,    packet->sz_payload, { packet->sz_payload });
        }
        }
        }

        outputs[0] = std::move(o);
    }

    void pack_extended_depth(int64_t frame_index, int32_t status, hl2ss::ulm::packet* packet, matlab::mex::ArgumentList outputs)
    {
        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "frame_index", "status", "timestamp", "depth", "pose" });

        o[0]["frame_index"] = m_factory.createScalar<int64_t>(frame_index);
        o[0]["status"]      = m_factory.createScalar<int32_t>(status);

        if (packet)
        {
        o[0]["timestamp"]   = m_factory.createScalar<uint64_t>(packet->timestamp);
        if (packet->payload)
        {
        hl2ss::map_extended_depth region = hl2ss::unpack_extended_depth(packet->payload, packet->sz_payload);

        uint32_t width  = region.metadata->width;
        uint32_t height = region.metadata->height;
        uint32_t size   = width * height * sizeof(uint16_t);

        o[0]["depth"]       = unpack_payload<uint16_t>(region.depth, 0, size, { height, width });
        }
        if (packet->pose)
        {
        o[0]["pose"]        = unpack_pose(packet->pose);
        }
        }

        outputs[0] = std::move(o);
    }

    std::shared_ptr<hl2ss::shared::packet_view> grab(hl2ss::shared::source* source, int64_t& frame_index, int32_t& status, matlab::mex::ArgumentList inputs)
    {
        if (!source) { throw std::runtime_error("Port not open"); }

        uint8_t type = get_argument<uint8_t>(inputs);
        std::unique_ptr<hl2ss::shared::packet_view> packet;

        if (type == hl2ss::matlab::grab_mode::BY_FRAME_INDEX)
        {
        int64_t index = get_argument<int64_t>(inputs);
        
        packet = source->get_by_index(index);
        }
        else if (type == hl2ss::matlab::grab_mode::BY_TIMESTAMP)
        {
        uint64_t timestamp       = get_argument<uint64_t>(inputs);
        int32_t  time_preference = get_argument<int32_t>(inputs);
        int32_t  tiebreak_right  = get_argument<int32_t>(inputs);

        packet = source->get_by_timestamp(timestamp, time_preference, tiebreak_right);
        }
        else
        {
        throw std::runtime_error("Unsupported grab type");
        }

        frame_index = packet->frame_stamp;
        status      = packet->status;
        
        return packet;
    }

    void get_packet_rm_vlc(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_rm_vlc[port - hl2ss::stream_port::RM_VLC_LEFTFRONT].get(), frame_index, status, inputs);
        pack_rm_vlc(frame_index, status, packet.get(), outputs);
    }

    void get_packet_rm_depth_ahat(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)port;
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_rm_depth_ahat.get(), frame_index, status, inputs);
        pack_rm_depth_ahat(frame_index, status, packet.get(), outputs);
    }

    void get_packet_rm_depth_longthrow(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)port;
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_rm_depth_longthrow.get(), frame_index, status, inputs);
        pack_rm_depth_longthrow(frame_index, status, packet.get(), outputs);
    }

    void get_packet_rm_imu(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_rm_imu[port - hl2ss::stream_port::RM_IMU_ACCELEROMETER].get(), frame_index, status, inputs);
        pack_rm_imu(frame_index, status, packet.get(), outputs);
    }

    void get_packet_pv(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        hl2ss::shared::source* source;
        switch (port)
        {
        case hl2ss::stream_port::PERSONAL_VIDEO: source = source_pv.get(); break;
        case hl2ss::stream_port::EXTENDED_VIDEO: source = source_ev.get(); break;
        default:                                 source = nullptr;         break;
        }
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source, frame_index, status, inputs);
        pack_pv(frame_index, status, packet.get(), outputs);
    }

    void get_packet_microphone(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)port;
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_microphone.get(), frame_index, status, inputs);
        pack_microphone(frame_index, status, packet.get(), configuration_microphone.profile, configuration_microphone.level, outputs);
    }

    void get_packet_si(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)port;
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_si.get(), frame_index, status, inputs);
        pack_si(frame_index, status, packet.get(), outputs);
    }

    void get_packet_eet(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)port;
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_eet.get(), frame_index, status, inputs);
        pack_eet(frame_index, status, packet.get(), outputs);
    }

    void get_packet_extended_audio(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)port;
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_extended_audio.get(), frame_index, status, inputs);
        pack_extended_audio(frame_index, status, packet.get(), configuration_extended_audio.profile, configuration_extended_audio.level, outputs);        
    }

    void get_packet_extended_depth(uint16_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)port;
        int64_t frame_index;
        int32_t status;
        std::shared_ptr<hl2ss::shared::packet_view> packet = grab(source_extended_depth.get(), frame_index, status, inputs);
        pack_extended_depth(frame_index, status, packet.get(), outputs);
    }

    void get_packet(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        uint16_t port = get_argument<uint16_t>(inputs);

        switch (port)
        {
        // Stream
        case hl2ss::stream_port::RM_VLC_LEFTFRONT:     
        case hl2ss::stream_port::RM_VLC_LEFTLEFT:      
        case hl2ss::stream_port::RM_VLC_RIGHTFRONT:    
        case hl2ss::stream_port::RM_VLC_RIGHTRIGHT:    get_packet_rm_vlc(            port, outputs, inputs); break;
        case hl2ss::stream_port::RM_DEPTH_AHAT:        get_packet_rm_depth_ahat(     port, outputs, inputs); break;
        case hl2ss::stream_port::RM_DEPTH_LONGTHROW:   get_packet_rm_depth_longthrow(port, outputs, inputs); break;
        case hl2ss::stream_port::RM_IMU_ACCELEROMETER: 
        case hl2ss::stream_port::RM_IMU_GYROSCOPE:     
        case hl2ss::stream_port::RM_IMU_MAGNETOMETER:  get_packet_rm_imu(            port, outputs, inputs); break;
        case hl2ss::stream_port::PERSONAL_VIDEO:       get_packet_pv(                port, outputs, inputs); break;
        case hl2ss::stream_port::MICROPHONE:           get_packet_microphone(        port, outputs, inputs); break;
        case hl2ss::stream_port::SPATIAL_INPUT:        get_packet_si(                port, outputs, inputs); break;
        case hl2ss::stream_port::EXTENDED_EYE_TRACKER: get_packet_eet(               port, outputs, inputs); break;
        case hl2ss::stream_port::EXTENDED_AUDIO:       get_packet_extended_audio(    port, outputs, inputs); break;
        case hl2ss::stream_port::EXTENDED_VIDEO:       get_packet_pv(                port, outputs, inputs); break;
        case hl2ss::stream_port::EXTENDED_DEPTH:       get_packet_extended_depth(    port, outputs, inputs); break;
        default:                                       throw std::runtime_error("Unsupported port");
        }
    }

    //------------------------------------------------------------------------------
    // (*) Control
    //------------------------------------------------------------------------------

    void start_subsystem_pv(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::string host                       = get_argument_string(inputs);
        uint16_t    port                       = get_argument<uint16_t>(inputs);

        auto configuration = hl2ss::ulm::configuration_pv_subsystem();

        configuration.enable_mrc                 = get_argument<bool>(inputs);
        configuration.hologram_composition       = get_argument<bool>(inputs);
        configuration.recording_indicator        = get_argument<bool>(inputs);
        configuration.video_stabilization        = get_argument<bool>(inputs);
        configuration.blank_protected            = get_argument<bool>(inputs);
        configuration.show_mesh                  = get_argument<bool>(inputs);
        configuration.shared                     = get_argument<bool>(inputs);
        configuration.global_opacity             = get_argument<float>(inputs);
        configuration.output_width               = get_argument<float>(inputs);
        configuration.output_height              = get_argument<float>(inputs);
        configuration.video_stabilization_length = get_argument<uint32_t>(inputs);
        configuration.hologram_perspective       = get_argument<uint32_t>(inputs);

        switch (port)
        {
        case hl2ss::stream_port::PERSONAL_VIDEO: if (source_pv)             { throw std::runtime_error("Cannot start subsystem while streaming"); } break;
        case hl2ss::stream_port::EXTENDED_VIDEO: if (source_ev)             { throw std::runtime_error("Cannot start subsystem while streaming"); } break;
        case hl2ss::stream_port::EXTENDED_DEPTH: if (source_extended_depth) { throw std::runtime_error("Cannot start subsystem while streaming"); } break;
        default:                                                              throw std::runtime_error("Unsupported operation");
        }

        hl2ss::svc::start_subsystem_pv(host.c_str(), port, &configuration);
    }

    void stop_subsystem_pv(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::string host = get_argument_string(inputs);
        uint16_t    port = get_argument<uint16_t>(inputs);

        switch (port)
        {
        case hl2ss::stream_port::PERSONAL_VIDEO: if (source_pv)             { throw std::runtime_error("Cannot stop subsystem while streaming"); } break;
        case hl2ss::stream_port::EXTENDED_VIDEO: if (source_ev)             { throw std::runtime_error("Cannot stop subsystem while streaming"); } break;
        case hl2ss::stream_port::EXTENDED_DEPTH: if (source_extended_depth) { throw std::runtime_error("Cannot stop subsystem while streaming"); } break;
        default:                                                              throw std::runtime_error("Unsupported operation");
        }

        hl2ss::svc::stop_subsystem_pv(host.c_str(), port);
    }

    //------------------------------------------------------------------------------
    // (*) Calibration
    //------------------------------------------------------------------------------

    void download_calibration_rm_vlc(char const* host, uint32_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)inputs;

        if (source_rm_vlc[port - hl2ss::stream_port::RM_VLC_LEFTFRONT]) { throw std::runtime_error("Cannot download calibration while streaming"); }

        auto calibration = hl2ss::svc::download_calibration<hl2ss::calibration_rm_vlc>(host, port, nullptr);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "uv2xy", "extrinsics", "undistort_map", "intrinsics" });

        o[0]["uv2xy"]         = unpack_payload<float>(calibration->data->uv2xy,         0, sizeof(calibration->data->uv2xy),         { 2, hl2ss::parameters_rm_vlc::HEIGHT, hl2ss::parameters_rm_vlc::WIDTH });
        o[0]["extrinsics"]    = unpack_payload<float>(calibration->data->extrinsics,    0, sizeof(calibration->data->extrinsics),    { 4, 4 });
        o[0]["undistort_map"] = unpack_payload<float>(calibration->data->undistort_map, 0, sizeof(calibration->data->undistort_map), { 2, hl2ss::parameters_rm_vlc::HEIGHT, hl2ss::parameters_rm_vlc::WIDTH });
        o[0]["intrinsics"]    = to_typed_array<float>(calibration->data->intrinsics,     { sizeof(calibration->data->intrinsics) / sizeof(float) });

        outputs[0] = std::move(o);
    }

    void download_calibration_rm_depth_ahat(char const* host, uint32_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)inputs;

        if (source_rm_depth_ahat) { throw std::runtime_error("Cannot download calibration while streaming"); }

        auto calibration = hl2ss::svc::download_calibration<hl2ss::calibration_rm_depth_ahat>(host, port, nullptr);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "uv2xy", "extrinsics", "scale", "alias", "undistort_map", "intrinsics" });

        o[0]["uv2xy"]         = unpack_payload<float>(        calibration->data->uv2xy,         0, sizeof(calibration->data->uv2xy),         { 2, hl2ss::parameters_rm_depth_ahat::HEIGHT, hl2ss::parameters_rm_depth_ahat::WIDTH });
        o[0]["extrinsics"]    = unpack_payload<float>(        calibration->data->extrinsics,    0, sizeof(calibration->data->extrinsics),    { 4, 4 });
        o[0]["scale"]         = m_factory.createScalar<float>(calibration->data->scale);
        o[0]["alias"]         = m_factory.createScalar<float>(calibration->data->alias);
        o[0]["undistort_map"] = unpack_payload<float>(        calibration->data->undistort_map, 0, sizeof(calibration->data->undistort_map), { 2, hl2ss::parameters_rm_depth_ahat::HEIGHT, hl2ss::parameters_rm_depth_ahat::WIDTH });
        o[0]["intrinsics"]    = to_typed_array<float>(        calibration->data->intrinsics,     { sizeof(calibration->data->intrinsics) / sizeof(float) });

        outputs[0] = std::move(o);
    }

    void download_calibration_rm_depth_longthrow(char const* host, uint32_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)inputs;

        if (source_rm_depth_longthrow) { throw std::runtime_error("Cannot download calibration while streaming"); }

        auto calibration = hl2ss::svc::download_calibration<hl2ss::calibration_rm_depth_longthrow>(host, port, nullptr);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "uv2xy", "extrinsics", "scale", "undistort_map", "intrinsics" });

        o[0]["uv2xy"]         = unpack_payload<float>(        calibration->data->uv2xy,         0, sizeof(calibration->data->uv2xy),         { 2, hl2ss::parameters_rm_depth_longthrow::HEIGHT, hl2ss::parameters_rm_depth_longthrow::WIDTH });
        o[0]["extrinsics"]    = unpack_payload<float>(        calibration->data->extrinsics,    0, sizeof(calibration->data->extrinsics),    { 4, 4 });
        o[0]["scale"]         = m_factory.createScalar<float>(calibration->data->scale);
        o[0]["undistort_map"] = unpack_payload<float>(        calibration->data->undistort_map, 0, sizeof(calibration->data->undistort_map), { 2, hl2ss::parameters_rm_depth_longthrow::HEIGHT, hl2ss::parameters_rm_depth_longthrow::WIDTH });
        o[0]["intrinsics"]    = to_typed_array<float>(        calibration->data->intrinsics,     { sizeof(calibration->data->intrinsics) / sizeof(float) });

        outputs[0] = std::move(o);
    }

    void download_calibration_rm_imu(char const* host, uint32_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        (void)inputs;

        if (source_rm_imu[port - hl2ss::stream_port::RM_IMU_ACCELEROMETER]) { throw std::runtime_error("Cannot download calibration while streaming"); }

        auto calibration = hl2ss::svc::download_calibration<hl2ss::calibration_rm_imu>(host, port, nullptr);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "extrinsics" });

        o[0]["extrinsics"] = unpack_payload<float>(calibration->data->extrinsics, 0, sizeof(calibration->data->extrinsics), { 4, 4 });

        outputs[0] = std::move(o);
    }

    void download_calibration_pv(char const* host, uint32_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (source_pv) { throw std::runtime_error("Cannot download calibration while streaming"); }

        auto configuration = hl2ss::ulm::configuration_pv();

        configuration.width     = get_argument<uint16_t>(inputs);
        configuration.height    = get_argument<uint16_t>(inputs);
        configuration.framerate = get_argument<uint8_t>(inputs);

        auto calibration = hl2ss::svc::download_calibration<hl2ss::calibration_pv>(host, port, &configuration);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "focal_length", "principal_point", "radial_distortion", "tangential_distortion", "projection", "extrinsics", "intrinsics_mf", "extrinsics_mf" });

        o[0]["focal_length"]          = to_typed_array<float>(calibration->data->focal_length,          { sizeof(calibration->data->focal_length)          / sizeof(float) });
        o[0]["principal_point"]       = to_typed_array<float>(calibration->data->principal_point,       { sizeof(calibration->data->principal_point)       / sizeof(float) });
        o[0]["radial_distortion"]     = to_typed_array<float>(calibration->data->radial_distortion,     { sizeof(calibration->data->radial_distortion)     / sizeof(float) });
        o[0]["tangential_distortion"] = to_typed_array<float>(calibration->data->tangential_distortion, { sizeof(calibration->data->tangential_distortion) / sizeof(float) });
        o[0]["projection"]            = unpack_payload<float>(calibration->data->projection,           0, sizeof(calibration->data->projection), { 4, 4 });
        o[0]["extrinsics"]            = unpack_payload<float>(calibration->data->extrinsics,           0, sizeof(calibration->data->extrinsics), { 4, 4 });
        o[0]["intrinsics_mf"]         = to_typed_array<float>(calibration->data->intrinsics_mf,         { sizeof(calibration->data->intrinsics_mf)         / sizeof(float) });
        o[0]["extrinsics_mf"]         = to_typed_array<float>(calibration->data->extrinsics_mf,         { sizeof(calibration->data->extrinsics_mf)         / sizeof(float) });

        outputs[0] = std::move(o);
    }

    void download_devicelist_extended_audio(char const* host, uint32_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        auto configuration = hl2ss::ulm::configuration_extended_audio();

        configuration.profile = get_argument<uint8_t>(inputs);
        configuration.level   = get_argument<uint8_t>(inputs);

        auto device_list = hl2ss::svc::download_device_list(host, port, &configuration);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "device_list" });

        o[0]["device_list"] = unpack_payload<uint8_t>(device_list->data, 0, device_list->size, { device_list->size });

        outputs[0] = std::move(o);
    }

    void download_devicelist_extended_video(char const* host, uint32_t port, matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        auto device_list = hl2ss::svc::download_device_list(host, port, nullptr);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "device_list" });

        o[0]["device_list"] = unpack_payload<uint8_t>(device_list->data, 0, device_list->size, { device_list->size });

        outputs[0] = std::move(o);
    }

    void download_calibration(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::string host = get_argument_string(inputs);
        uint16_t    port = get_argument<uint16_t>(inputs);

        switch (port)
        {
        // Stream
        case hl2ss::stream_port::RM_VLC_LEFTFRONT:     
        case hl2ss::stream_port::RM_VLC_LEFTLEFT:      
        case hl2ss::stream_port::RM_VLC_RIGHTFRONT:    
        case hl2ss::stream_port::RM_VLC_RIGHTRIGHT:    download_calibration_rm_vlc(            host.c_str(), port, outputs, inputs); break;
        case hl2ss::stream_port::RM_DEPTH_AHAT:        download_calibration_rm_depth_ahat(     host.c_str(), port, outputs, inputs); break;
        case hl2ss::stream_port::RM_DEPTH_LONGTHROW:   download_calibration_rm_depth_longthrow(host.c_str(), port, outputs, inputs); break;
        case hl2ss::stream_port::RM_IMU_ACCELEROMETER: 
        case hl2ss::stream_port::RM_IMU_GYROSCOPE:     download_calibration_rm_imu(            host.c_str(), port, outputs, inputs); break;
        case hl2ss::stream_port::PERSONAL_VIDEO:       download_calibration_pv(                host.c_str(), port, outputs, inputs); break;
        case hl2ss::stream_port::EXTENDED_AUDIO:       download_devicelist_extended_audio(     host.c_str(), port, outputs, inputs); break;
        case hl2ss::stream_port::EXTENDED_VIDEO:       download_devicelist_extended_video(     host.c_str(), port, outputs, inputs); break;
        default:                                       throw std::runtime_error("Unsupported port");
        }
    }

    //------------------------------------------------------------------------------
    // (*) IPC
    //------------------------------------------------------------------------------

    void ipc_call_rc(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (!ipc_rc) { throw std::runtime_error("Port not open"); }

        std::string f = get_argument_string(inputs);

        if (f == "ee_get_application_version")
        {
        outputs[0] = to_typed_array<uint16_t>(ipc_rc->ee_get_application_version().field, { 4 });
        }
        else if (f == "ts_get_utc_offset")
        {
        outputs[0] = m_factory.createScalar<uint64_t>(ipc_rc->ts_get_utc_offset());
        }
        else if (f == "hs_set_marker_state")
        {
        uint32_t state = get_argument<uint32_t>(inputs);
        ipc_rc->hs_set_marker_state(state);
        }
        else if (f == "pv_get_subsystem_status")
        {
        outputs[0] = m_factory.createScalar<bool>(ipc_rc->pv_get_subsystem_status());
        }
        else if (f == "pv_set_focus")
        {
        uint32_t mode            = get_argument<uint32_t>(inputs);
        uint32_t range           = get_argument<uint32_t>(inputs);
        uint32_t distance        = get_argument<uint32_t>(inputs);
        uint32_t value           = get_argument<uint32_t>(inputs);
        uint32_t driver_fallback = get_argument<uint32_t>(inputs);

        ipc_rc->pv_set_focus(mode, range, distance, value, driver_fallback);
        }
        else if (f == "pv_set_video_temporal_denoising")
        {
        uint32_t mode = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_video_temporal_denoising(mode);
        }
        else if (f == "pv_set_white_balance_preset")
        {
        uint32_t preset = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_white_balance_preset(preset);
        }
        else if (f == "pv_set_white_balance_value")
        {
        uint32_t value = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_white_balance_value(value);
        }
        else if (f == "pv_set_exposure")
        {
        uint32_t mode  = get_argument<uint32_t>(inputs);
        uint32_t value = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_exposure(mode, value);
        }
        else if (f == "pv_set_exposure_priority_video")
        {
        uint32_t enabled = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_exposure_priority_video(enabled);
        }
        else if (f == "pv_set_iso_speed")
        {
        uint32_t mode  = get_argument<uint32_t>(inputs);
        uint32_t value = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_iso_speed(mode, value);
        }
        else if (f == "pv_set_backlight_compensation")
        {
        uint32_t state = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_backlight_compensation(state);
        }       
        else if (f == "pv_set_scene_mode")
        {
        uint32_t mode = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_scene_mode(mode);
        }
        else if (f == "ee_set_flat_mode")
        {
        uint32_t value = get_argument<uint32_t>(inputs);
        ipc_rc->ee_set_flat_mode(value);
        }
        else if (f == "rm_set_eye_selection")
        {
        uint32_t enable = get_argument<uint32_t>(inputs);
        ipc_rc->rm_set_eye_selection(enable);
        }
        else if (f == "pv_set_desired_optimization")
        {
        uint32_t mode = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_desired_optimization(mode);
        }
        else if (f == "pv_set_primary_use")
        {
        uint32_t mode = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_primary_use(mode);
        }
        else if (f == "pv_set_optical_image_stabilization")
        {
        uint32_t mode = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_optical_image_stabilization(mode);
        }
        else if (f == "pv_set_hdr_video")
        {
        uint32_t mode = get_argument<uint32_t>(inputs);
        ipc_rc->pv_set_hdr_video(mode);
        }
        else if (f == "pv_set_regions_of_interest")
        {
        bool     clear             = get_argument<bool>(inputs);
        bool     set               = get_argument<bool>(inputs);
        bool     auto_exposure     = get_argument<bool>(inputs);
        bool     auto_focus        = get_argument<bool>(inputs);
        bool     bounds_normalized = get_argument<bool>(inputs);
        uint32_t type              = get_argument<uint32_t>(inputs);
        uint32_t weight            = get_argument<uint32_t>(inputs);
        float    x                 = get_argument<float>(inputs);
        float    y                 = get_argument<float>(inputs);
        float    w                 = get_argument<float>(inputs);
        float    h                 = get_argument<float>(inputs);
        ipc_rc->pv_set_regions_of_interest(clear, set, auto_exposure, auto_focus, bounds_normalized, type, weight, x, y, w, h);
        }
        else if (f == "ee_set_interface_priority")
        {
        uint16_t port     = get_argument<uint16_t>(inputs);
        int32_t  priority = get_argument<int32_t>(inputs);
        ipc_rc->ee_set_interface_priority(port, priority);
        }
        else if (f == "ee_set_quiet_mode")
        {
        uint32_t mode = get_argument<uint32_t>(inputs);
        ipc_rc->ee_set_quiet_mode(mode);
        }
        else if (f == "rm_map_camera_points")
        {
        uint16_t                        port      = get_argument<uint16_t>(inputs);
        uint32_t                        operation = get_argument<uint32_t>(inputs);
        matlab::data::TypedArray<float> points    = get_argument_array<float>(inputs);
        uint32_t                        count     = (uint32_t)points.getNumberOfElements() / 2;
        auto result = ipc_rc->rm_map_camera_points(port, operation, (hl2ss::vector_2*)get_pointer(points), count);
        outputs[0] = to_typed_array<float>(result->data, count * 2 * sizeof(float), { 2 * count });
        }
        else if (f == "rm_get_rignode_world_poses")
        {
        matlab::data::TypedArray<uint64_t> timestamps = get_argument_array<uint64_t>(inputs);
        uint32_t                           count      = (uint32_t)timestamps.getNumberOfElements();
        auto result = ipc_rc->rm_get_rignode_world_poses(get_pointer(timestamps), count);
        outputs[0] = to_typed_array<float>(result->data, count * sizeof(hl2ss::matrix_4x4), { 4, 4, count });
        }
        else if (f == "ts_get_current_time")
        {
        uint32_t source = get_argument<uint32_t>(inputs);
        outputs[0] = m_factory.createScalar<uint64_t>(ipc_rc->ts_get_current_time(source));
        }
        else if (f == "si_set_sampling_delay")
        {
        int64_t delay = get_argument<int64_t>(inputs);
        ipc_rc->si_set_sampling_delay(delay);
        }
        else if (f == "ee_set_encoder_buffering")
        {
        bool enable = get_argument<bool>(inputs);
        ipc_rc->ee_set_encoder_buffering(enable);
        }
        else if (f == "ee_set_reader_buffering")
        {
        bool enable = get_argument<bool>(inputs);
        ipc_rc->ee_set_reader_buffering(enable);
        }
        else if (f == "rm_set_loop_control")
        {
        uint16_t port   = get_argument<uint16_t>(inputs);
        bool     enable = get_argument<bool>(inputs);
        ipc_rc->rm_set_loop_control(port, enable);
        }
        else
        {
        throw std::runtime_error("Unknown method");
        }
    }

    void ipc_call_sm(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (!ipc_sm) { throw std::runtime_error("Port not open"); }

        std::string f = get_argument_string(inputs);

        if (f == "set_volumes")
        {
        matlab::data::StructArray p = get_argument_array<matlab::data::Struct>(inputs);

        hl2ss::sm_bounding_volume volumes;

        for (size_t i = 0; i < p.getNumberOfElements(); ++i)
        {
        switch (get_field_scalar<uint32_t>(p[i]["type"]))
        {
        case hl2ss::sm_volume_type::Box:         volumes.add_box({ get_field_vector_3(p[i]["center"]), get_field_vector_3(p[i]["extents"]) }); break;
        case hl2ss::sm_volume_type::Frustum:     volumes.add_frustum({ get_field_vector_4(p[i]["near"]), get_field_vector_4(p[i]["far"]), get_field_vector_4(p[i]["right"]), get_field_vector_4(p[i]["left"]), get_field_vector_4(p[i]["top"]), get_field_vector_4(p[i]["bottom"]) }); break;
        case hl2ss::sm_volume_type::OrientedBox: volumes.add_oriented_box({ get_field_vector_3(p[i]["center"]), get_field_vector_3(p[i]["extents"]), get_field_vector_4(p[i]["orientation"]) }); break;
        case hl2ss::sm_volume_type::Sphere:      volumes.add_sphere({ get_field_vector_3(p[i]["center"]), get_field_scalar<float>(p[i]["radius"]) }); break;
        default: break;
        }
        }

        ipc_sm->set_volumes(volumes);
        }
        else if (f == "get_observed_surfaces")
        {
        auto surface_infos = ipc_sm->get_observed_surfaces();

        matlab::data::StructArray o = m_factory.createStructArray({ surface_infos->size }, { "id", "update_time" });

        for (size_t i = 0; i < surface_infos->size; ++i)
        {
        auto& info = surface_infos->data[i];

        o[i]["id"]          = to_typed_array<uint64_t>(&info.id, sizeof(info.id), { sizeof(info.id) / sizeof(uint64_t) });
        o[i]["update_time"] = m_factory.createScalar<uint64_t>(info.update_time);
        }

        outputs[0] = std::move(o);
        }
        else if (f == "get_meshes")
        {
        matlab::data::StructArray p = get_argument_array<matlab::data::Struct>(inputs);

        hl2ss::sm_mesh_task tasks;

        for (size_t i = 0; i < p.getNumberOfElements(); ++i)
        {
        tasks.add_task(get_field_guid(p[i]["id"]), get_field_scalar<double>(p[i]["max_triangles_per_cubic_meter"]), get_field_scalar<uint32_t>(p[i]["vertex_position_format"]), get_field_scalar<uint32_t>(p[i]["triangle_index_format"]), get_field_scalar<uint32_t>(p[i]["vertex_normal_format"]));
        }

        auto result = ipc_sm->get_meshes(tasks);

        matlab::data::StructArray o = m_factory.createStructArray({ result->meshes.size() }, { "status", "vertex_position_scale", "pose", "bounds", "vertex_positions", "triangle_indices", "vertex_normals" });

        for (size_t i = 0; i < result->meshes.size(); ++i)
        {
        auto& mesh = result->meshes[i];

        o[i]["status"]                = m_factory.createScalar<uint32_t>(mesh.status);
        o[i]["vertex_position_scale"] = to_typed_array<float>(&mesh.vertex_position_scale, sizeof(mesh.vertex_position_scale), { sizeof(mesh.vertex_position_scale) / sizeof(float) });
        o[i]["pose"]                  = unpack_pose(&mesh.pose);
        o[i]["bounds"]                = to_typed_array<float>( mesh.bounds_data,                  mesh.bounds_size,            {        mesh.bounds_size            / sizeof(float) });

        switch (get_field_scalar<uint32_t>(p[i]["vertex_position_format"]))
        {
        case hl2ss::sm_vertex_position_format::R16G16B16A16IntNormalized: o[i]["vertex_positions"] = to_typed_array<uint16_t>(mesh.vertex_positions_data, mesh.vertex_positions_size, { 4, mesh.vertex_positions_size / (4 * sizeof(uint16_t)) }); break;
        case hl2ss::sm_vertex_position_format::R32G32B32A32Float:         o[i]["vertex_positions"] = to_typed_array<float>(   mesh.vertex_positions_data, mesh.vertex_positions_size, { 4, mesh.vertex_positions_size / (4 * sizeof(float)) });    break;
        }

        switch (get_field_scalar<uint32_t>(p[i]["triangle_index_format"]))
        {
        case hl2ss::sm_triangle_index_format::R16UInt: o[i]["triangle_indices"] = to_typed_array<uint16_t>(mesh.triangle_indices_data, mesh.triangle_indices_size, { 3, mesh.triangle_indices_size / (3 * sizeof(uint16_t)) }); break;
        case hl2ss::sm_triangle_index_format::R32Uint: o[i]["triangle_indices"] = to_typed_array<uint32_t>(mesh.triangle_indices_data, mesh.triangle_indices_size, { 3, mesh.triangle_indices_size / (3 * sizeof(uint32_t)) }); break;
        }

        switch (get_field_scalar<uint32_t>(p[i]["vertex_normal_format"]))
        {
        case hl2ss::sm_vertex_normal_format::R8G8B8A8IntNormalized: o[i]["vertex_normals"] = to_typed_array<uint8_t>(mesh.vertex_normals_data, mesh.vertex_normals_size, { 4, mesh.vertex_normals_size / (4 * sizeof(uint8_t))}); break;
        case hl2ss::sm_vertex_normal_format::R32G32B32A32Float:     o[i]["vertex_normals"] = to_typed_array<float>(  mesh.vertex_normals_data, mesh.vertex_normals_size, { 4, mesh.vertex_normals_size / (4 * sizeof(float))});   break;
        }
        }

        outputs[0] = std::move(o);
        }
        else
        {
        throw std::runtime_error("Unknown method");
        }
    }

    void ipc_call_su(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (!ipc_su) { throw std::runtime_error("Port not open"); }

        std::string f = get_argument_string(inputs);

        if (f == "query")
        {
        matlab::data::StructArray p = get_argument_array<matlab::data::Struct>(inputs);

        hl2ss::su_task task;
        
        task.enable_quads         = get_field_scalar<bool>(p[0]["enable_quads"]);
        task.enable_meshes        = get_field_scalar<bool>(p[0]["enable_meshes"]);
        task.enable_only_observed = get_field_scalar<bool>(p[0]["enable_only_observed"]);
        task.enable_world_mesh    = get_field_scalar<bool>(p[0]["enable_world_mesh"]);
        task.mesh_lod             = get_field_scalar<uint32_t>(p[0]["mesh_lod"]);
        task.query_radius         = get_field_scalar<float>(p[0]["query_radius"]);
        task.create_mode          = get_field_scalar<uint8_t>(p[0]["create_mode"]);
        task.kind_flags           = get_field_scalar<uint8_t>(p[0]["kind_flags"]);
        task.get_orientation      = get_field_scalar<bool>(p[0]["get_orientation"]);
        task.get_position         = get_field_scalar<bool>(p[0]["get_position"]);
        task.get_location_matrix  = get_field_scalar<bool>(p[0]["get_location_matrix"]);
        task.get_quad             = get_field_scalar<bool>(p[0]["get_quad"]);
        task.get_meshes           = get_field_scalar<bool>(p[0]["get_meshes"]); 
        task.get_collider_meshes  = get_field_scalar<bool>(p[0]["get_collider_meshes"]);

        matlab::data::TypedArray<uint64_t> guid_list = p[0]["guid_list"];
        for (size_t i = 0; i < (guid_list.getNumberOfElements() & ~1ULL); i += 2) { task.guid_list.push_back({ guid_list[i], guid_list[i + 1] }); }

        auto result = ipc_su->query(task);

        matlab::data::StructArray o = m_factory.createStructArray({ 1 }, { "status", "extrinsics", "pose", "items" });

        o[0]["status"]     = m_factory.createScalar<uint32_t>(result->status);
        o[0]["extrinsics"] = unpack_payload<float>(&result->extrinsics, 0, sizeof(result->extrinsics), { 4, 4 });
        o[0]["pose"]       = unpack_payload<float>(&result->pose,       0, sizeof(result->pose),       { 4, 4 });

        if (result->items.size() > 0)
        {
        matlab::data::StructArray items = m_factory.createStructArray({ result->items.size() }, { "id", "kind", "orientation", "position", "location", "alignment", "extents", "meshes", "collider_meshes" });
        
        for (size_t i = 0; i < result->items.size(); ++i)
        {
        auto& item = result->items[i];

        items[i]["id"]          = to_typed_array<uint64_t>(&item.id,          sizeof(item.id),          { sizeof(item.id)          / sizeof(uint64_t) });
        items[i]["kind"]        = m_factory.createScalar<int32_t>(item.kind);
        items[i]["orientation"] = to_typed_array<float>(   &item.orientation, sizeof(item.orientation), { sizeof(item.orientation) / sizeof(float) });
        items[i]["position"]    = to_typed_array<float>(   &item.position,    sizeof(item.position),    { sizeof(item.position)    / sizeof(float) });
        items[i]["location"]    = unpack_pose(&item.location);
        items[i]["alignment"]   = m_factory.createScalar<int32_t>(item.alignment);
        items[i]["extents"]     = to_typed_array<float>(   &item.extents,     sizeof(item.extents),     { sizeof(item.extents)     / sizeof(float) });

        if (item.unpacked_meshes.size() > 0)
        {
        matlab::data::StructArray meshes = m_factory.createStructArray({ item.unpacked_meshes.size() }, { "vertex_positions", "triangle_indices" });

        for (size_t j = 0; j < item.unpacked_meshes.size(); ++j)
        {
        auto& mesh = item.unpacked_meshes[j];

        meshes[j]["vertex_positions"] = to_typed_array<float>(   mesh.vertex_positions_data, mesh.vertex_positions_size, { 3, mesh.vertex_positions_size / (3 * sizeof(float)) });
        meshes[j]["triangle_indices"] = to_typed_array<uint32_t>(mesh.triangle_indices_data, mesh.triangle_indices_size, { 3, mesh.triangle_indices_size / (3 * sizeof(uint32_t)) });
        }

        items[i]["meshes"] = std::move(meshes);
        }

        if (item.unpacked_collider_meshes.size() > 0)
        {
        matlab::data::StructArray collider_meshes = m_factory.createStructArray({ item.unpacked_collider_meshes.size() }, { "vertex_positions", "triangle_indices" });

        for (size_t j = 0; j < item.unpacked_collider_meshes.size(); ++j)
        {
        auto &collider_mesh = item.unpacked_collider_meshes[j];

        collider_meshes[j]["vertex_positions"] = to_typed_array<float>(   collider_mesh.vertex_positions_data, collider_mesh.vertex_positions_size, { 3, collider_mesh.vertex_positions_size / (3 * sizeof(float)) });
        collider_meshes[j]["triangle_indices"] = to_typed_array<uint32_t>(collider_mesh.triangle_indices_data, collider_mesh.triangle_indices_size, { 3, collider_mesh.triangle_indices_size / (3 * sizeof(uint32_t)) });
        }

        items[i]["collider_meshes"] = std::move(collider_meshes);
        }
        }

        o[0]["items"] = std::move(items);   
        }

        outputs[0] = std::move(o);
        }
        else
        {
        throw std::runtime_error("Unknown method");
        }
    }

    void ipc_call_vi(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (!ipc_vi) { throw std::runtime_error("Port not open"); }

        std::string f = get_argument_string(inputs);

        if (f == "start")
        {
        matlab::data::StringArray commands = get_argument_array<matlab::data::MATLABString>(inputs);

        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
        std::vector<char> utf8_s;
        
        for (size_t i = 0; i < commands.getNumberOfElements(); ++i)
        {
        if (commands[i].has_value())
        {
        auto command = convert.to_bytes(commands[i]);
        utf8_s.insert(utf8_s.end(), command.begin(), command.end());
        utf8_s.push_back(0);
        }
        }
        utf8_s.push_back(0);

        ipc_vi->start(utf8_s.data());
        }
        else if (f == "pop")
        {
        auto results = ipc_vi->pop();

        matlab::data::StructArray o = m_factory.createStructArray({ results->size }, { "index", "confidence", "phrase_duration", "phrase_start_time", "raw_confidence" });

        for (size_t i = 0; i < results->size; ++i)
        {
        auto& result = results->data[i];

        o[i]["index"]             = m_factory.createScalar<uint32_t>(result.index);
        o[i]["confidence"]        = m_factory.createScalar<uint32_t>(result.confidence);
        o[i]["phrase_duration"]   = m_factory.createScalar<uint64_t>(result.phrase_duration);
        o[i]["phrase_start_time"] = m_factory.createScalar<uint64_t>(result.phrase_start_time);
        o[i]["raw_confidence"]    = m_factory.createScalar<double>(  result.raw_confidence);
        }

        outputs[0] = std::move(o);
        }
        else if (f == "stop")
        {
        ipc_vi->stop();
        }
        else
        {
        throw std::runtime_error("Unknown method");
        }
    }

    void ipc_call_umq(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (!ipc_umq) { throw std::runtime_error("Port not open"); }

        std::string f = get_argument_string(inputs);

        if (f == "push")
        {
        matlab::data::TypedArray<uint8_t> data = get_argument_array<uint8_t>(inputs);
        ipc_umq->push(get_pointer(data), data.getNumberOfElements());
        }
        else if (f == "pull")
        {
        uint32_t count = get_argument<uint32_t>(inputs);
        matlab::data::TypedArray<uint32_t> data = m_factory.createArray<uint32_t>({ count });
        ipc_umq->pull(get_pointer(data), count);
        outputs[0] = std::move(data);
        }
        else
        {
        throw std::runtime_error("Unknown method");
        }
    }

    void ipc_call_gmq(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (!ipc_gmq) { throw std::runtime_error("Port not open"); }

        std::string f = get_argument_string(inputs);

        if (f == "pull")
        {
        auto msg = ipc_gmq->pull();
        matlab::data::TypedArray<uint8_t> data = m_factory.createArray<uint8_t>({ msg->size });
        if (msg->size > 0) { memcpy(get_pointer(data), msg->data, msg->size); }
        outputs[0] = m_factory.createScalar<uint32_t>(msg->command);
        outputs[1] = std::move(data);
        }
        else if (f == "push")
        {
        matlab::data::TypedArray<uint32_t> response = get_argument_array<uint32_t>(inputs);
        ipc_gmq->push(get_pointer(response), response.getNumberOfElements());
        }
        else
        {
        throw std::runtime_error("Unknown method");
        }
    }

    void ipc_call(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        uint16_t port = get_argument<uint16_t>(inputs);

        switch (port)
        {
        case hl2ss::ipc_port::REMOTE_CONFIGURATION: ipc_call_rc( outputs, inputs); break;
        case hl2ss::ipc_port::SPATIAL_MAPPING:      ipc_call_sm( outputs, inputs); break;
        case hl2ss::ipc_port::SCENE_UNDERSTANDING:  ipc_call_su( outputs, inputs); break;
        case hl2ss::ipc_port::VOICE_INPUT:          ipc_call_vi( outputs, inputs); break;
        case hl2ss::ipc_port::UNITY_MESSAGE_QUEUE:  ipc_call_umq(outputs, inputs); break;
        case hl2ss::ipc_port::GUEST_MESSAGE_QUEUE:  ipc_call_gmq(outputs, inputs); break;
        default:                                    throw std::runtime_error("Unsupported port");
        }
    }

    //------------------------------------------------------------------------------
    // (*) Entry
    //------------------------------------------------------------------------------

    MexFunction()
    {
        try
        {
        hl2ss::svc::initialize();
        m_initialized = true;
        }
        catch (const std::exception& e)
        {
        m_initialized = false;
        }
    }

    ~MexFunction()
    {
        if (m_initialized) { hl2ss::svc::cleanup(); }
    }

    void select(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::string action = get_argument_string(inputs);

        if      (action == "get_packet")           { get_packet(outputs, inputs); }
        else if (action == "ipc_call")             { ipc_call(outputs, inputs); }
        else if (action == "open")                 { open(outputs, inputs); }
        else if (action == "close")                { close(outputs, inputs); }
        else if (action == "start_subsystem_pv")   { start_subsystem_pv(outputs, inputs); }
        else if (action == "stop_subsystem_pv")    { stop_subsystem_pv(outputs, inputs); }
        else if (action == "download_calibration") { download_calibration(outputs, inputs); }
        else                                       { throw std::runtime_error("Unknown action"); }
    }

    void operator() (matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        m_argument_index = 0;
        try { select(outputs, inputs); } catch (const std::exception& e) { error(e.what()); }
    }
};
