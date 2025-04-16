from smolagents import Tool, tool

class NetworkTools(Tool):
    name = "network_tools"
    description = "Tools for network discovery and topology mapping using ping3"

    def __init__(self):
        self.is_initialized = True
  
    @tool
    def measure_connection_speed(target: str) -> float:
        """
        Measures connection speed (RTT) to target using ping3.
        Args:
            target (str): Target IP to measure connection to
        Returns:
            Round trip time in milliseconds
        """
        import ping3
        import statistics
        
        # Take multiple measurements for accuracy
        ping_results = []
        for _ in range(5):
            delay = ping3.ping(target)
            if delay is not None:
                ping_results.append(delay * 1000)  # Convert to ms
        
        if ping_results:
            return statistics.mean(ping_results)
        return float(-1)
    
