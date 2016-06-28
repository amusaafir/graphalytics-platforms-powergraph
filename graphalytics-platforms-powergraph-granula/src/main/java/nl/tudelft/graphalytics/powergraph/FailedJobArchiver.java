package nl.tudelft.graphalytics.powergraph;

import nl.tudelft.granula.modeller.job.JobModel;
import nl.tudelft.granula.modeller.platform.Powergraph;
import nl.tudelft.graphalytics.granula.GranulaManager;

import java.io.IOException;

/**
 * Created by wlngai on 6/20/16.
 */
public class FailedJobArchiver {

    public static void main(String[] args) {
        String rawlogPath = args[0];
        String archPath = "./iffailed";
        String benchmarkIdString = args[1];
        String benchmarkId = args[2];
        long startTime = Long.parseLong(args[3]);
        long endTime = System.currentTimeMillis();

        JobModel jobModel =  new JobModel(new Powergraph());

        try {
            GranulaManager.generateFailedJobArchive(rawlogPath, archPath, benchmarkIdString, benchmarkId, startTime, endTime, jobModel);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
